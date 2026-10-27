#include "extensions/clang-format/PostFormatSource.h"

#include "TokenAnalyzer.h"
#include "TokenAnnotator.h"

#include <unordered_map>
#include <vector>

namespace clang::format::extensions {
namespace {

struct TokenFacts {
  bool BracedInit = false;
  bool RequiresExpression = false;
  bool LambdaBody = false;
  bool TemplateOpener = false;
  bool TemplateCloser = false;
  bool LineStart = false;
};

// Collects the parser's brace and template classification by file offset.
// Raw tokens decide statement boundaries, because the parser can merge
// statements across preprocessor lines (for example after a misparsed C++26
// pack-indexed return type), but only the parser knows whether a brace starts
// a braced initializer and whether `<` opens a template argument list.
class TokenFactCollector : public TokenAnalyzer {
public:
  TokenFactCollector(const Environment &Env, const FormatStyle &Style,
                     std::unordered_map<unsigned, TokenFacts> &Facts)
      : TokenAnalyzer(Env, Style), Facts(Facts) {}

  std::pair<tooling::Replacements, unsigned>
  analyze(TokenAnnotator &, SmallVectorImpl<AnnotatedLine *> &Lines,
          FormatTokenLexer &) override {
    for (const AnnotatedLine *Line : Lines) {
      if (!Line->InPPDirective)
        facts(*Line->First).LineStart = true;
      record(*Line);
    }
    return {};
  }

private:
  TokenFacts &facts(const FormatToken &Token) {
    return Facts[Env.getSourceManager().getFileOffset(
        Token.Tok.getLocation())];
  }

  void record(const AnnotatedLine &Line) {
    for (const FormatToken *Token = Line.First; Token; Token = Token->Next) {
      if (Token->is(tok::l_brace) && Token->is(BK_BracedInit))
        facts(*Token).BracedInit = true;
      if (Token->is(TT_RequiresExpressionLBrace))
        facts(*Token).RequiresExpression = true;
      if (Token->is(TT_LambdaLBrace))
        facts(*Token).LambdaBody = true;
      if (Token->is(TT_TemplateOpener))
        facts(*Token).TemplateOpener = true;
      if (Token->is(TT_TemplateCloser))
        facts(*Token).TemplateCloser = true;
      for (const AnnotatedLine *Child : Token->Children) {
        facts(*Child->First).LineStart = true;
        record(*Child);
      }
    }
  }

  std::unordered_map<unsigned, TokenFacts> &Facts;
};

enum class ItemKind { Directive, Comment, AccessSpecifier, Statement };

struct Item {
  ItemKind Kind;
  unsigned First;
  unsigned Last;
  bool Definition = false;
  // A statement without `;` or a body that ends where the parser starts a new
  // line, such as a decorating macro (`MIDL_INTERFACE(...)`).
  bool Unterminated = false;
};

enum class BodyKind { Namespace, Linkage, Type, Function };

class DefinitionSeparator {
public:
  DefinitionSeparator(const PostFormatSource &Source,
                      const std::unordered_map<unsigned, TokenFacts> &Facts,
                      AffectedRangeManager &AffectedRangeMgr,
                      std::vector<PendingReplacement> &Pending)
      : Source(Source), Tokens(Source.Tokens), Facts(Facts),
        AffectedRangeMgr(AffectedRangeMgr), Pending(Pending) {}

  void run() { separateScope(0, Tokens.size(), false); }

private:
  const TokenFacts *facts(unsigned Index) const {
    auto It = Facts.find(Tokens[Index].Offset);
    return It == Facts.end() ? nullptr : &It->second;
  }

  bool inDirective(unsigned Index) const {
    return Source.Lines[Tokens[Index].LineIndex].InPPDirective;
  }

  bool onCommentOnlyLine(unsigned Index) const {
    return Source.Lines[Tokens[Index].LineIndex].CommentOnly;
  }

  unsigned lastLine(unsigned Index) const {
    return Tokens[Index].LineIndex +
           static_cast<unsigned>(Tokens[Index].Text.count('\n'));
  }

  bool isAccessKeyword(unsigned Index) const {
    const StringRef Text = Tokens[Index].Text;
    return Text == "public" || Text == "protected" || Text == "private";
  }

  // Parses the items of the scope [Begin, End) and separates its definitions.
  // In a block scope only multi-line lambda definitions are definitions.
  void separateScope(unsigned Begin, unsigned End, bool Block) {
    std::vector<Item> Items;
    for (unsigned I = Begin; I < End;) {
      Item Next = parseItem(I, End, Block);
      I = Next.Last + 1;
      // A trailing comment belongs to the code before it.
      while (I < End && Tokens[I].Tok.is(tok::comment) &&
             Tokens[I].LineIndex == Tokens[Next.Last].LineIndex) {
        Next.Last = I++;
      }
      Items.push_back(Next);
    }
    separateItems(Items);
  }

  Item parseItem(unsigned I, unsigned End, bool Block) {
    if (inDirective(I)) {
      unsigned Last = I;
      while (Last + 1 < End && inDirective(Last + 1) &&
             !(Tokens[Last + 1].Tok.is(tok::hash) &&
               Tokens[Last + 1].LineIndex != Tokens[Last].LineIndex &&
               Source.Lines[Tokens[Last + 1].LineIndex].FirstNonCommentToken ==
                   Last + 1)) {
        ++Last;
      }
      return {ItemKind::Directive, I, Last};
    }
    if (Tokens[I].Tok.is(tok::comment) && onCommentOnlyLine(I))
      return {ItemKind::Comment, I, I};
    if (isAccessKeyword(I)) {
      unsigned Last = I + 1;
      while (Last < End && Tokens[Last].Tok.isNot(tok::colon) &&
             Tokens[Last].LineIndex == Tokens[I].LineIndex) {
        ++Last;
      }
      if (Last < End && Tokens[Last].Tok.is(tok::colon))
        return {ItemKind::AccessSpecifier, I, Last};
    }
    return parseStatement(I, End, Block);
  }

  Item parseStatement(unsigned First, unsigned End, bool Block) {
    Item Statement{ItemKind::Statement, First, First};
    int ParenDepth = 0;
    int TemplateDepth = 0;
    bool SawAssignment = false;
    bool SawTypeKeyword = false;
    bool SawEnum = false;
    bool StartsNamespace = false;
    bool StartsLinkage = false;
    for (unsigned I = First; I < End; ++I) {
      if (inDirective(I))
        continue;
      const TokenInfo &Token = Tokens[I];
      Statement.Last = I;
      if (I != First && ParenDepth == 0 && TemplateDepth == 0 &&
          Token.Tok.isNot(tok::l_brace) && Token.Tok.isNot(tok::r_brace) &&
          Token.Tok.isNot(tok::comment) && Token.Text != "try" &&
          Token.Text != "catch") {
        if (const TokenFacts *Facts = facts(I); Facts && Facts->LineStart) {
          // Own-line comments before the next line belong to what follows.
          Statement.Last = I - 1;
          while (Statement.Last > First &&
                 (inDirective(Statement.Last) ||
                  (Tokens[Statement.Last].Tok.is(tok::comment) &&
                   onCommentOnlyLine(Statement.Last)))) {
            --Statement.Last;
          }
          Statement.Unterminated = true;
          return Statement;
        }
      }
      if (const TokenFacts *Facts = facts(I)) {
        TemplateDepth += Facts->TemplateOpener;
        TemplateDepth -= Facts->TemplateCloser;
      }
      if (Token.Tok.is(tok::greatergreater) && TemplateDepth > 0) {
        const auto Second = Facts.find(Token.Offset + 1);
        if (Second != Facts.end() && Second->second.TemplateCloser)
          --TemplateDepth;
      }
      if (Token.Tok.isOneOf(tok::l_paren, tok::l_square)) {
        ++ParenDepth;
        continue;
      }
      if (Token.Tok.isOneOf(tok::r_paren, tok::r_square)) {
        --ParenDepth;
        continue;
      }
      if (ParenDepth != 0 || TemplateDepth != 0)
        continue;
      const StringRef Text = Token.Text;
      if (I == First && Text == "namespace")
        StartsNamespace = true;
      if (I == First + 1 && Text == "namespace" &&
          Tokens[First].Text == "inline") {
        StartsNamespace = true;
      }
      if (I == First + 1 && Token.Tok.is(tok::string_literal) &&
          Tokens[First].Text == "extern") {
        StartsLinkage = true;
      }
      SawTypeKeyword |= Text == "class" || Text == "struct" ||
                        Text == "union";
      SawEnum |= Text == "enum";
      SawAssignment |= Token.Tok.is(tok::equal) &&
                       Tokens[I - 1].Text != "operator";
      if (Token.Tok.is(tok::semi))
        return Statement;
      if (Token.Tok.isNot(tok::l_brace))
        continue;

      const int Close = Source.MatchingBraces[I];
      if (Close < 0 || static_cast<unsigned>(Close) >= End) {
        Statement.Last = End - 1;
        return Statement;
      }
      const TokenFacts *Facts = facts(I);
      const bool BracedInit = Facts && Facts->BracedInit;
      const bool Body = !SawAssignment &&
                        !(Facts && Facts->RequiresExpression) &&
                        (SawEnum || !BracedInit || StartsNamespace);
      if (!Body) {
        if (Facts && Facts->LambdaBody) {
          Statement.Definition |=
              SawAssignment && lastLine(static_cast<unsigned>(Close)) !=
                                   Tokens[I].LineIndex;
          separateScope(I + 1, static_cast<unsigned>(Close), true);
        }
        I = static_cast<unsigned>(Close);
        continue;
      }

      BodyKind Kind = BodyKind::Function;
      if (StartsNamespace)
        Kind = BodyKind::Namespace;
      else if (StartsLinkage && I == First + 2)
        Kind = BodyKind::Linkage;
      else if (SawTypeKeyword || SawEnum)
        Kind = BodyKind::Type;
      if (!SawEnum) {
        separateScope(I + 1, static_cast<unsigned>(Close),
                      Block || Kind == BodyKind::Function);
      }
      Statement.Definition |= !Block && Kind != BodyKind::Linkage;
      Statement.Last = static_cast<unsigned>(Close);
      I = static_cast<unsigned>(Close);
      // Types end at the semicolon after their declarators; function try
      // blocks continue with their handlers.
      if (Kind == BodyKind::Type)
        continue;
      const int After = nextNonCommentToken(Tokens, Close);
      if (After >= 0 && static_cast<unsigned>(After) < End) {
        if (Tokens[After].Text == "catch")
          continue;
        if (Tokens[After].Tok.is(tok::semi))
          Statement.Last = static_cast<unsigned>(After);
      }
      return Statement;
    }
    return Statement;
  }

  void separateItems(std::vector<Item> &Items) {
    // Comment and decorating macro lines directly above a definition belong
    // to it.
    for (size_t I = Items.size(); I-- > 1;) {
      const Item &Previous = Items[I - 1];
      const bool Attaches =
          Previous.Kind == ItemKind::Comment ||
          (Previous.Kind == ItemKind::Statement && Previous.Unterminated &&
           !Previous.Definition);
      if (!Items[I].Definition || !Attaches ||
          Tokens[Items[I].First].LineIndex != lastLine(Previous.Last) + 1) {
        continue;
      }
      Items[I].First = Items[I - 1].First;
      Items.erase(Items.begin() + static_cast<long>(I - 1));
    }

    for (size_t I = 0; I < Items.size(); ++I) {
      if (!Items[I].Definition)
        continue;
      if (I > 0 && Items[I - 1].Kind != ItemKind::AccessSpecifier)
        queueBlankLineBetween(Items[I - 1].Last, Items[I].First);
      if (I + 1 < Items.size())
        queueBlankLineBetween(Items[I].Last, Items[I + 1].First);
    }
  }

  void queueBlankLineBetween(unsigned Before, unsigned After) {
    const TokenInfo &Previous = Tokens[Before];
    const TokenInfo &Next = Tokens[After];
    if (Previous.Disabled || Next.Disabled ||
        !rangeAffected(AffectedRangeMgr, Source.StartOfFile,
                       Previous.EndOffset, Next.Offset, Next.Offset,
                       Next.EndOffset)) {
      return;
    }
    queueSingleBlankLine(Pending, Source.Code, Previous.EndOffset, Next.Offset,
                         Source.DefaultLineEnding,
                         "SeparateDefinitionBlocksFromAll");
  }

  const PostFormatSource &Source;
  const std::vector<TokenInfo> &Tokens;
  const std::unordered_map<unsigned, TokenFacts> &Facts;
  AffectedRangeManager &AffectedRangeMgr;
  std::vector<PendingReplacement> &Pending;
};

} // namespace

void collectDefinitionSeparationFixes(const Environment &Env,
                                      const FormatStyle &Style,
                                      const PostFormatSource &Source,
                                      AffectedRangeManager &AffectedRangeMgr,
                                      std::vector<PendingReplacement> &Pending) {
  std::unordered_map<unsigned, TokenFacts> Facts;
  TokenFactCollector(Env, Style, Facts).process();
  DefinitionSeparator(Source, Facts, AffectedRangeMgr, Pending).run();
}

} // namespace clang::format::extensions
