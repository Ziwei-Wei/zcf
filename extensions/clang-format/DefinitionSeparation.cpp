#include "extensions/clang-format/PostFormatSource.h"

#include "extensions/clang-format/ExtensionStyle.h"

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
  // A function declaration's name or parameter list opener.
  bool FunctionDeclaration = false;
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
  analyze(TokenAnnotator &Annotator, SmallVectorImpl<AnnotatedLine *> &Lines,
          FormatTokenLexer &) override {
    for (AnnotatedLine *Line : Lines) {
      // Function declaration parentheses are classified here.
      Annotator.calculateFormattingInformation(*Line);
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
      if (Token->isOneOf(TT_FunctionDeclarationName,
                         TT_FunctionDeclarationLParen)) {
        facts(*Token).FunctionDeclaration = true;
      }
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
  // A function declaration without a body, such as `int f();`.
  bool FunctionDeclaration = false;
};

enum class BodyKind { Namespace, Linkage, Type, Function };

class DefinitionSeparator {
public:
  DefinitionSeparator(const PostFormatSource &Source,
                      const std::unordered_map<unsigned, TokenFacts> &Facts,
                      const ExtensionStyle &Extensions, bool BeforeFormatting,
                      unsigned TabWidth, AffectedRangeManager &AffectedRangeMgr,
                      std::vector<PendingReplacement> &Pending)
      : Source(Source), Tokens(Source.Tokens), Facts(Facts),
        Extensions(Extensions), BeforeFormatting(BeforeFormatting),
        TabWidth(TabWidth), AffectedRangeMgr(AffectedRangeMgr),
        Pending(Pending) {}

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
      // So do the lines LibFormat wrapped that comment onto.
      const unsigned Continued =
          lastTrailingCommentContinuation(Source, Next.Last, TabWidth);
      if (Continued > Next.Last && Continued < End) {
        Next.Last = Continued;
        I = Continued + 1;
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
    bool SawFunctionDeclaration = false;
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
      if (ParenDepth == 0 && TemplateDepth == 0) {
        const TokenFacts *Facts = facts(I);
        SawFunctionDeclaration |= Facts && Facts->FunctionDeclaration;
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
      if (Token.Tok.is(tok::semi)) {
        Statement.FunctionDeclaration = SawFunctionDeclaration && !Block;
        return Statement;
      }
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
              !BeforeFormatting && SawAssignment &&
              lastLine(static_cast<unsigned>(Close)) != Tokens[I].LineIndex;
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

  enum class ConditionalKind { None, Open, Branch, Close };

  ConditionalKind conditionalKind(const Item &Directive) const {
    if (Directive.Kind != ItemKind::Directive ||
        Directive.First == Directive.Last ||
        Tokens[Directive.First].Tok.isNot(tok::hash)) {
      return ConditionalKind::None;
    }
    const StringRef Name = Tokens[Directive.First + 1].Text;
    if (Name == "if" || Name == "ifdef" || Name == "ifndef")
      return ConditionalKind::Open;
    if (Name == "elif" || Name == "elifdef" || Name == "elifndef" ||
        Name == "else") {
      return ConditionalKind::Branch;
    }
    if (Name == "endif")
      return ConditionalKind::Close;
    return ConditionalKind::None;
  }

  // Returns the index of the directive closing the conditional opened at
  // Open, or End when it is not closed before End.
  size_t findConditionalClose(const std::vector<Item> &Items, size_t Open,
                              size_t End) const {
    unsigned Depth = 0;
    for (size_t I = Open + 1; I < End; ++I) {
      const ConditionalKind Kind = conditionalKind(Items[I]);
      if (Kind == ConditionalKind::Open) {
        ++Depth;
      } else if (Kind == ConditionalKind::Close) {
        if (Depth == 0)
          return I;
        --Depth;
      }
    }
    return End;
  }

  // Separates definitions and function declarations from every neighbor,
  // including directive lines, and conditional groups from their neighbors.
  void separateItems(std::vector<Item> &Items) {
    // Comment and decorating macro lines directly above a definition belong
    // to it.
    for (size_t I = Items.size(); I-- > 1;) {
      const Item &Previous = Items[I - 1];
      const bool Attaches =
          Previous.Kind == ItemKind::Comment ||
          (Previous.Kind == ItemKind::Statement && Previous.Unterminated &&
           !Previous.Definition);
      if (!separable(Items[I]) || !Attaches ||
          Tokens[Items[I].First].LineIndex != lastLine(Previous.Last) + 1) {
        continue;
      }
      Items[I].First = Items[I - 1].First;
      Items.erase(Items.begin() + static_cast<long>(I - 1));
    }

    for (size_t I = 0; I < Items.size(); ++I) {
      if (!separable(Items[I]))
        continue;
      if (I > 0 && Items[I - 1].Kind != ItemKind::AccessSpecifier)
        queueBlankLineBetween(Items[I - 1].Last, Items[I].First);
      if (I + 1 < Items.size())
        queueBlankLineBetween(Items[I].Last, Items[I + 1].First);
    }

    if (Extensions.SeparatePreprocessorConditionalBlocks)
      separateConditionalGroups(Items, 0, Items.size());
  }

  bool separable(const Item &Candidate) const {
    return (Candidate.Definition &&
            Extensions.SeparateDefinitionBlocksFromAll) ||
           (Candidate.FunctionDeclaration &&
            Extensions.SeparateFunctionDeclarations);
  }

  // Separates each balanced `#if`...`#endif` group in [Begin, End) from its
  // neighbors, then the groups nested in each of its branches. Comment lines
  // directly above `#if` stay attached to the group.
  void separateConditionalGroups(const std::vector<Item> &Items, size_t Begin,
                                 size_t End) {
    for (size_t I = Begin; I < End; ++I) {
      if (conditionalKind(Items[I]) != ConditionalKind::Open)
        continue;
      const size_t Close = findConditionalClose(Items, I, End);
      if (Close == End)
        continue;
      size_t Leading = I;
      while (Leading > Begin &&
             Items[Leading - 1].Kind == ItemKind::Comment &&
             Tokens[Items[Leading].First].LineIndex ==
                 lastLine(Items[Leading - 1].Last) + 1) {
        --Leading;
      }
      if (Leading > Begin &&
          Items[Leading - 1].Kind != ItemKind::AccessSpecifier) {
        queueBlankLineBetween(Items[Leading - 1].Last, Items[Leading].First);
      }
      if (Close + 1 < End)
        queueBlankLineBetween(Items[Close].Last, Items[Close + 1].First);

      size_t BranchBegin = I + 1;
      for (size_t J = I + 1; J <= Close; ++J) {
        const ConditionalKind Kind = conditionalKind(Items[J]);
        if (Kind == ConditionalKind::Open) {
          J = findConditionalClose(Items, J, Close);
          continue;
        }
        if (Kind == ConditionalKind::None)
          continue;
        separateConditionalGroups(Items, BranchBegin, J);
        BranchBegin = J + 1;
      }
      I = Close;
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
                         "DefinitionSeparation");
  }

  const PostFormatSource &Source;
  const std::vector<TokenInfo> &Tokens;
  const std::unordered_map<unsigned, TokenFacts> &Facts;
  const ExtensionStyle &Extensions;
  // Lambda layout is unknown before formatting, so lambdas are left to the
  // post-format pass.
  const bool BeforeFormatting;
  const unsigned TabWidth;
  AffectedRangeManager &AffectedRangeMgr;
  std::vector<PendingReplacement> &Pending;
};

} // namespace

void collectDefinitionSeparationFixes(const Environment &Env,
                                      const FormatStyle &Style,
                                      const PostFormatSource &Source,
                                      const ExtensionStyle &Extensions,
                                      bool BeforeFormatting,
                                      AffectedRangeManager &AffectedRangeMgr,
                                      std::vector<PendingReplacement> &Pending) {
  std::unordered_map<unsigned, TokenFacts> Facts;
  TokenFactCollector(Env, Style, Facts).process();
  DefinitionSeparator(Source, Facts, Extensions, BeforeFormatting,
                      std::max(Style.TabWidth, 1u), AffectedRangeMgr, Pending)
      .run();
}

} // namespace clang::format::extensions
