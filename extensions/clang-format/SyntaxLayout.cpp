#include "extensions/clang-format/ExtensionPasses.h"

#include "ContinuationIndenter.h"
#include "FormatToken.h"
#include "TokenAnnotator.h"
#include "integration/clang-format/ExtensionContext.h"

#include <algorithm>
#include <optional>

namespace clang::format::extensions {
namespace {

bool isNonEmptyParen(const FormatToken &LeftParen) {
  return LeftParen.is(tok::l_paren) && LeftParen.MatchingParen &&
         LeftParen.Next && LeftParen.Next != LeftParen.MatchingParen;
}

bool hasCallCallee(const FormatToken &LeftParen) {
  const auto *Callee = LeftParen.getPreviousNonComment();
  return Callee && Callee->isOneOf(tok::identifier, tok::r_paren, tok::r_square,
                                   TT_TemplateCloser);
}

bool isCallParen(const AnnotatedLine &Line, const FormatToken &LeftParen) {
  if (!isNonEmptyParen(LeftParen) ||
      LeftParen.isOneOf(TT_AttributeLParen, TT_ConditionLParen,
                        TT_FunctionDeclarationLParen, TT_FunctionTypeLParen,
                        TT_LambdaDefinitionLParen,
                        TT_OverloadedOperatorLParen) ||
      Line.InPPDirective) {
    return false;
  }

  return hasCallCallee(LeftParen);
}

const FormatToken *findEnclosingCallParen(const AnnotatedLine &Line,
                                          const FormatToken &Current) {
  if (Current.is(tok::r_paren) && Current.MatchingParen &&
      isCallParen(Line, *Current.MatchingParen)) {
    return Current.MatchingParen;
  }
  for (const auto *Token = Current.Previous; Token; Token = Token->Previous) {
    if (Token->isNot(tok::l_paren) || !Token->MatchingParen ||
        !isCallParen(Line, *Token))
      continue;
    for (const auto *Inside = Token->Next;
         Inside && Inside != Token->MatchingParen; Inside = Inside->Next) {
      if (Inside == &Current)
        return Token;
    }
  }
  return nullptr;
}

bool hasOneSimpleArgument(const FormatToken &LeftParen) {
  if (!isNonEmptyParen(LeftParen))
    return false;
  const auto *RightParen = LeftParen.MatchingParen;
  for (const auto *Token = LeftParen.Next; Token && Token != RightParen;
       Token = Token->Next) {
    if (Token->is(tok::comma) &&
        Token->NestingLevel == LeftParen.NestingLevel + 1) {
      return false;
    }
    if (Token->isOneOf(tok::comment, tok::semi))
      return false;
  }
  return true;
}

void compactSingleArgumentCall(FormatToken &LeftParen) {
  auto *RightParen = LeftParen.MatchingParen;
  for (auto *Token = LeftParen.Next; Token; Token = Token->Next) {
    Token->MustBreakBefore = false;
    Token->NewlinesBefore = 0;
    if (Token == RightParen)
      break;
  }
}

void forceMultilineList(FormatToken &LeftParen) {
  auto *RightParen = LeftParen.MatchingParen;
  if (!RightParen || !LeftParen.Next || LeftParen.Next == RightParen)
    return;
  LeftParen.Next->MustBreakBefore = true;
  LeftParen.Next->CanBreakBefore = true;
  for (auto *Token = LeftParen.Next; Token && Token != RightParen;
       Token = Token->Next) {
    if (Token->is(tok::comma) && Token->Next &&
        Token->NestingLevel == LeftParen.NestingLevel + 1) {
      Token->Next->MustBreakBefore = true;
      Token->Next->CanBreakBefore = true;
    }
  }
  RightParen->MustBreakBefore = true;
  RightParen->CanBreakBefore = true;
}

void breakConstructorDestructorSpecifier(AnnotatedLine &Line) {
  for (auto *Specifier = Line.First; Specifier; Specifier = Specifier->Next) {
    if (!Specifier->isOneOf(tok::kw_constexpr, tok::kw_consteval,
                            tok::kw_virtual)) {
      continue;
    }
    auto *Name = Specifier->getNextNonComment();
    if (!Name || Name->isOneOf(tok::l_paren, tok::semi))
      continue;
    if (Specifier->isOneOf(tok::kw_constexpr, tok::kw_consteval) &&
        Name->isNot(TT_CtorDtorDeclName)) {
      continue;
    }
    if (Specifier->is(tok::kw_virtual) &&
        !Name->isOneOf(tok::tilde, TT_CtorDtorDeclName)) {
      continue;
    }
    Name->MustBreakBefore = true;
    Name->CanBreakBefore = true;
    Name->NewlinesBefore = std::max(Name->NewlinesBefore, 1u);
    if (Specifier->is(tok::kw_virtual))
      Line.First->NewlinesBefore = std::max(Line.First->NewlinesBefore, 2u);
  }
}

void expandNestedAggregate(AnnotatedLine &Line,
                           const FormatStyle &FormatStyle) {
  unsigned MaximumBraceLevel = 0;
  unsigned MinimumBraceLevel = 0;
  unsigned BraceCount = 0;
  bool HasAssignment = false;
  bool SawBrace = false;
  for (const auto *Token = Line.First; Token; Token = Token->Next) {
    HasAssignment |= Token->is(tok::equal);
    if (Token->is(tok::l_brace)) {
      ++BraceCount;
      if (!SawBrace) {
        MinimumBraceLevel = Token->NestingLevel;
        SawBrace = true;
      } else {
        MinimumBraceLevel = std::min(MinimumBraceLevel, Token->NestingLevel);
      }
      MaximumBraceLevel = std::max(MaximumBraceLevel, Token->NestingLevel);
    }
  }
  if (!HasAssignment || BraceCount < 3)
    return;
  const unsigned BraceLevels = MaximumBraceLevel - MinimumBraceLevel + 1;
  if (FormatStyle.AlignArrayOfStructures != FormatStyle::AIAS_None &&
      BraceLevels == 2) {
    return;
  }

  for (auto *Token = Line.First; Token; Token = Token->Next) {
    if (Token->is(tok::l_brace)) {
      const auto *Previous = Token->getPreviousNonComment();
      const bool IsRoot = Previous && Previous->is(tok::equal);
      const bool StartsNestedLevel = Previous && Previous->is(tok::l_brace);
      const bool StartsOuterSibling = Previous && Previous->is(tok::comma) &&
                                      Token->NestingLevel < MaximumBraceLevel;
      if (IsRoot || StartsNestedLevel || StartsOuterSibling) {
        Token->MustBreakBefore = true;
        Token->CanBreakBefore = true;
      }
      if (Token->Next && Token->Next->isNot(tok::r_brace)) {
        Token->Next->MustBreakBefore = true;
        Token->Next->CanBreakBefore = true;
      }
    } else if (Token->is(tok::r_brace) && Token->MatchingParen) {
      Token->MustBreakBefore = true;
      Token->CanBreakBefore = true;
    } else if (Token->is(tok::comma) && Token->Next &&
               Token->NestingLevel > MaximumBraceLevel) {
      Token->Next->MustBreakBefore = false;
      Token->Next->NewlinesBefore = 0;
    }
  }
}

void breakRequiresExpression(AnnotatedLine &Line) {
  for (auto *Token = Line.First; Token; Token = Token->Next) {
    if (Token->is(TT_RequiresExpressionLBrace)) {
      Token->MustBreakBefore = true;
      Token->CanBreakBefore = true;
      if (!Token->Children.empty() && Token->Children.front()->First) {
        Token->Children.front()->First->MustBreakBefore = true;
        Token->Children.front()->First->CanBreakBefore = true;
      }
      if (Token->MatchingParen) {
        Token->MatchingParen->MustBreakBefore = true;
        Token->MatchingParen->CanBreakBefore = true;
      }
    }
  }
}

void completePackEllipsisLayout(AnnotatedLine &Line) {
  bool HasPackIndex = false;
  bool HasPackBase = false;
  FormatToken *InheritanceColon = nullptr;
  for (auto *Token = Line.First; Token; Token = Token->Next) {
    if (Token->is(TT_InheritanceColon))
      InheritanceColon = Token;
    if (InheritanceColon && Token->is(tok::ellipsis))
      HasPackBase = true;
    HasPackIndex |= Token->is(tok::ellipsis) && Token->Next &&
                    Token->Next->is(tok::l_square);
  }
  if (HasPackBase && InheritanceColon && InheritanceColon->Next) {
    InheritanceColon->Next->MustBreakBefore = true;
    InheritanceColon->Next->CanBreakBefore = true;
  }
  if (!HasPackIndex)
    return;
  for (auto *Token = Line.First; Token; Token = Token->Next) {
    if (Token->is(tok::l_brace)) {
      Token->MustBreakBefore = true;
      Token->CanBreakBefore = true;
    }
  }
}

void breakFunctionPointerParameters(AnnotatedLine &Line) {
  for (auto *Token = Line.First; Token; Token = Token->Next) {
    if (Token->isNot(tok::l_paren) || !Token->Previous ||
        Token->Previous->isNot(tok::r_paren) ||
        !Token->Previous->MatchingParen) {
      continue;
    }
    bool HasPointer = false;
    for (const auto *Inner = Token->Previous->MatchingParen->Next;
         Inner && Inner != Token->Previous; Inner = Inner->Next) {
      HasPointer |= Inner->isPointerOrReference();
    }
    if (HasPointer)
      forceMultilineList(*Token);
  }
}

bool isArithmeticOperator(const FormatToken &Token) {
  return Token.is(TT_BinaryOperator) &&
         Token.isOneOf(tok::plus, tok::minus, tok::star, tok::slash,
                       tok::percent);
}

bool isAdditiveOperator(const FormatToken &Token) {
  return isArithmeticOperator(Token) && Token.isOneOf(tok::plus, tok::minus);
}

bool isMultiplicativeOperator(const FormatToken &Token) {
  return isArithmeticOperator(Token) &&
         Token.isOneOf(tok::star, tok::slash, tok::percent);
}

bool isAssignmentOperator(const FormatToken &Token) {
  return Token.isOneOf(tok::equal, tok::plusequal, tok::minusequal,
                       tok::starequal, tok::slashequal, tok::percentequal,
                       tok::ampequal, tok::pipeequal, tok::caretequal,
                       tok::lesslessequal, tok::greatergreaterequal);
}

bool isUnsafeArithmeticToken(const FormatToken &Token) {
  return Token.Finalized || Token.is(tok::comment) || Token.IsMultiline ||
         !Token.Children.empty();
}

std::optional<unsigned> getFlatWidth(const FormatToken &Begin,
                                     const FormatToken &End) {
  unsigned Width = 0;
  bool First = true;
  for (const auto *Token = &Begin; Token; Token = Token->Next) {
    if (isUnsafeArithmeticToken(*Token))
      return std::nullopt;
    Width += Token->ColumnWidth;
    if (!First)
      Width += Token->SpacesRequiredBefore;
    First = false;
    if (Token == &End)
      return Width;
  }
  return std::nullopt;
}

bool rangeContainsArithmetic(const FormatToken &Begin, const FormatToken &End) {
  for (const auto *Token = &Begin; Token; Token = Token->Next) {
    if (isArithmeticOperator(*Token))
      return true;
    if (Token == &End)
      break;
  }
  return false;
}

void forceBreakBefore(FormatToken &Token) {
  Token.MustBreakBefore = true;
  Token.CanBreakBefore = true;
  Token.NewlinesBefore = std::max(Token.NewlinesBefore, 1u);
}

void forceLeadingOperatorBreak(FormatToken &Operator) {
  forceBreakBefore(Operator);
  if (Operator.Next && Operator.Next->isNot(tok::comment)) {
    Operator.Next->MustBreakBefore = false;
    Operator.Next->CanBreakBefore = false;
    Operator.Next->NewlinesBefore = 0;
  }
}

std::optional<unsigned> getFlattenedWidth(const FormatToken &Begin,
                                          const FormatToken &End) {
  unsigned Width = 0;
  bool First = true;
  for (const auto *Token = &Begin; Token; Token = Token->Next) {
    if (Token->Finalized || Token->is(tok::comment) || Token->IsMultiline)
      return std::nullopt;
    if (!First)
      Width += Token->SpacesRequiredBefore;
    Width += Token->ColumnWidth;
    First = false;

    for (const AnnotatedLine *Child : Token->Children) {
      if (!Child || !Child->First || !Child->Last)
        return std::nullopt;
      const auto ChildWidth = getFlattenedWidth(*Child->First, *Child->Last);
      if (!ChildWidth)
        return std::nullopt;
      Width += 1 + *ChildWidth;
    }

    if (Token == &End)
      return Width;
  }
  return std::nullopt;
}

void countStatementSemicolonsOnPhysicalLines(const AnnotatedLine &Line,
                                             unsigned &CurrentLineCount,
                                             unsigned &MaximumLineCount) {
  const unsigned RootNesting = Line.First->NestingLevel;
  for (const auto *Token = Line.First; Token; Token = Token->Next) {
    if (Token->HasUnescapedNewline)
      CurrentLineCount = 0;
    if (Token->is(tok::semi) && Token->NestingLevel == RootNesting) {
      ++CurrentLineCount;
      MaximumLineCount = std::max(MaximumLineCount, CurrentLineCount);
    }
    for (const AnnotatedLine *Child : Token->Children) {
      if (Child)
        countStatementSemicolonsOnPhysicalLines(*Child, CurrentLineCount,
                                                MaximumLineCount);
    }
  }
}

FormatToken *findLambdaBodyBrace(FormatToken &LeftParen) {
  if (LeftParen.isNot(TT_LambdaDefinitionLParen) || !LeftParen.MatchingParen) {
    return nullptr;
  }

  for (auto *Token = LeftParen.MatchingParen->Next; Token;
       Token = Token->Next) {
    if (Token->is(TT_LambdaLBrace) && Token->MatchingParen)
      return Token;
    if (Token->NestingLevel < LeftParen.NestingLevel ||
        (Token->NestingLevel == LeftParen.NestingLevel &&
         Token->isOneOf(tok::semi, tok::comma))) {
      break;
    }
  }
  return nullptr;
}

const FormatToken *findLambdaCapture(const FormatToken &LeftParen) {
  const auto *CaptureClose = LeftParen.getPreviousNonComment();
  if (!CaptureClose || CaptureClose->isNot(tok::r_square) ||
      !CaptureClose->MatchingParen ||
      CaptureClose->MatchingParen->isNot(TT_LambdaLSquare)) {
    return nullptr;
  }
  return CaptureClose->MatchingParen;
}

bool lambdaNeedsExpansion(const FormatToken &LeftParen,
                          const FormatToken &BodyBrace,
                          const FormatStyle &FormatStyle, unsigned BaseIndent) {
  if (LeftParen.Next && LeftParen.MatchingParen &&
      LeftParen.Next->NewlinesBefore > 0 &&
      LeftParen.MatchingParen->NewlinesBefore > 0 &&
      BodyBrace.NewlinesBefore > 0) {
    return true;
  }

  unsigned CurrentLineSemicolons = 0;
  unsigned MaximumLineSemicolons = 0;
  for (const AnnotatedLine *Child : BodyBrace.Children) {
    if (Child)
      countStatementSemicolonsOnPhysicalLines(*Child, CurrentLineSemicolons,
                                              MaximumLineSemicolons);
  }
  if (MaximumLineSemicolons > 1)
    return true;
  if (FormatStyle.ColumnLimit == 0 || !BodyBrace.MatchingParen)
    return false;

  const auto *Capture = findLambdaCapture(LeftParen);
  if (!Capture)
    return false;
  const auto Width = getFlattenedWidth(*Capture, *BodyBrace.MatchingParen);
  return Width && BaseIndent + *Width > FormatStyle.ColumnLimit;
}

void expandBodyDrivenLambdas(AnnotatedLine &Line,
                             const FormatStyle &FormatStyle) {
  if (Line.InPPDirective || Line.InMacroBody)
    return;

  for (auto *Token = Line.First; Token; Token = Token->Next) {
    if (Token->isNot(TT_LambdaDefinitionLParen) || !isNonEmptyParen(*Token)) {
      continue;
    }
    auto *BodyBrace = findLambdaBodyBrace(*Token);
    if (!BodyBrace ||
        !lambdaNeedsExpansion(*Token, *BodyBrace, FormatStyle,
                              Line.Level * FormatStyle.IndentWidth)) {
      continue;
    }

    forceMultilineList(*Token);
    forceBreakBefore(*BodyBrace);
    for (AnnotatedLine *Child : BodyBrace->Children) {
      if (Child && Child->First)
        forceBreakBefore(*Child->First);
    }
    if (BodyBrace->MatchingParen)
      forceBreakBefore(*BodyBrace->MatchingParen);
  }
}

bool isTernaryRangeBoundary(const FormatToken &Token, unsigned NestingLevel) {
  if (Token.NestingLevel < NestingLevel)
    return true;
  if (Token.NestingLevel != NestingLevel)
    return false;
  return isAssignmentOperator(Token) ||
         Token.isOneOf(tok::comma, tok::semi, tok::kw_return, tok::kw_co_return,
                       tok::l_brace, tok::r_brace);
}

std::pair<FormatToken *, FormatToken *>
findTernaryRange(FormatToken &Question) {
  auto *Begin = Question.Previous;
  while (Begin && Begin->Previous &&
         !isTernaryRangeBoundary(*Begin->Previous, Question.NestingLevel)) {
    Begin = Begin->Previous;
  }

  auto *End = Question.Next;
  while (End && End->Next &&
         !isTernaryRangeBoundary(*End->Next, Question.NestingLevel)) {
    End = End->Next;
  }
  return {Begin, End};
}

bool ternaryRangeExceedsColumnLimit(const AnnotatedLine &Line,
                                    const FormatToken &Begin,
                                    const FormatToken &End,
                                    const FormatStyle &FormatStyle) {
  if (FormatStyle.ColumnLimit == 0)
    return false;
  const auto Width = getFlatWidth(Begin, End);
  const unsigned ContinuationColumn = Line.Level * FormatStyle.IndentWidth +
                                      FormatStyle.ContinuationIndentWidth;
  const unsigned AvailableWidth =
      FormatStyle.ColumnLimit > ContinuationColumn
          ? FormatStyle.ColumnLimit - ContinuationColumn
          : 1;
  return Width && *Width > AvailableWidth;
}

void expandOverflowingTernaries(AnnotatedLine &Line,
                                const FormatStyle &FormatStyle) {
  if (Line.InPPDirective || Line.InMacroBody) {
    return;
  }

  for (auto *Question = Line.First; Question;) {
    if (!Question->is(TT_ConditionalExpr) || Question->isNot(tok::question)) {
      Question = Question->Next;
      continue;
    }

    auto [Begin, End] = findTernaryRange(*Question);
    if (!Begin || !End) {
      Question = Question->Next;
      continue;
    }
    if (ternaryRangeExceedsColumnLimit(Line, *Begin, *End, FormatStyle)) {
      forceBreakBefore(*Begin);
      for (auto *Token = Begin; Token; Token = Token->Next) {
        if (Token->is(TT_ConditionalExpr)) {
          forceBreakBefore(*Token);
          auto *Operand = Token->getNextNonComment();
          if (Operand &&
              !(Token->is(tok::question) && Operand->is(tok::colon))) {
            forceBreakBefore(*Operand);
          }
        }
        if (Token == End)
          break;
      }
    }
    Question = End->Next;
  }
}

void expandOverflowingNestedTemplates(AnnotatedLine &Line,
                                      const FormatStyle &FormatStyle) {
  if (FormatStyle.ColumnLimit == 0 || Line.InPPDirective || Line.InMacroBody) {
    return;
  }

  for (auto *Root = Line.First; Root; Root = Root->Next) {
    if (Root->isNot(TT_TemplateOpener) || !Root->MatchingParen)
      continue;

    bool ContainsNestedTemplate = false;
    for (const auto *Token = Root->Next; Token && Token != Root->MatchingParen;
         Token = Token->Next) {
      if (Token->is(TT_TemplateOpener) && Token->MatchingParen) {
        ContainsNestedTemplate = true;
        break;
      }
    }
    if (!ContainsNestedTemplate)
      continue;

    const auto Width = getFlatWidth(*Root, *Root->MatchingParen);
    if (!Width || Line.Level * FormatStyle.IndentWidth + *Width <=
                      FormatStyle.ColumnLimit) {
      continue;
    }

    for (auto *Opener = Root; Opener; Opener = Opener->Next) {
      if (Opener->is(TT_TemplateOpener) && Opener->MatchingParen &&
          Opener->Next && Opener->Next != Opener->MatchingParen) {
        forceBreakBefore(*Opener->Next);
        for (auto *Token = Opener->Next;
             Token && Token != Opener->MatchingParen; Token = Token->Next) {
          if (Token->is(tok::comma) &&
              Token->NestingLevel == Opener->NestingLevel + 1 && Token->Next) {
            forceBreakBefore(*Token->Next);
          }
        }
        forceBreakBefore(*Opener->MatchingParen);
      }
      if (Opener == Root->MatchingParen)
        break;
    }
  }
}

bool isExpandedLambdaParen(const FormatToken &LeftParen) {
  return LeftParen.is(TT_LambdaDefinitionLParen) &&
         isNonEmptyParen(LeftParen) && LeftParen.Next->MustBreakBefore &&
         LeftParen.MatchingParen->MustBreakBefore;
}

const FormatToken *findExpandedLambdaParen(const FormatToken &BodyBrace) {
  for (const auto *Token = BodyBrace.getPreviousNonComment(); Token;) {
    if (Token->is(tok::r_paren) && Token->MatchingParen &&
        isExpandedLambdaParen(*Token->MatchingParen)) {
      return Token->MatchingParen;
    }
    if (Token->MatchingParen &&
        Token->isOneOf(tok::r_paren, tok::r_square, tok::r_brace,
                       TT_TemplateCloser)) {
      Token = Token->MatchingParen->getPreviousNonComment();
      continue;
    }
    if (Token->NestingLevel < BodyBrace.NestingLevel ||
        (Token->NestingLevel == BodyBrace.NestingLevel &&
         Token->isOneOf(tok::semi, tok::comma, tok::l_brace))) {
      break;
    }
    Token = Token->getPreviousNonComment();
  }
  return nullptr;
}

const FormatToken *findEnclosingExpandedLambda(const AnnotatedLine &Line,
                                               const FormatToken &Current) {
  for (const auto *LeftParen = Line.First; LeftParen;
       LeftParen = LeftParen->Next) {
    if (!isExpandedLambdaParen(*LeftParen) || !LeftParen->MatchingParen)
      continue;
    for (const auto *Inside = LeftParen->Next;
         Inside && Inside != LeftParen->MatchingParen; Inside = Inside->Next) {
      if (Inside == &Current)
        return LeftParen;
    }
  }
  return nullptr;
}

bool isExpandedVerticalTernaryContinuation(const FormatToken &Token) {
  if (!Token.MustBreakBefore)
    return false;
  if (Token.is(TT_ConditionalExpr))
    return true;

  const auto *Previous = Token.getPreviousNonComment();
  if (!Previous)
    return false;
  if (Previous->is(TT_ConditionalExpr))
    return Previous->MustBreakBefore;
  if (!isAssignmentOperator(*Previous) &&
      !Previous->isOneOf(tok::kw_return, tok::kw_co_return)) {
    return false;
  }

  for (const auto *Current = &Token; Current; Current = Current->Next) {
    if (Current->is(TT_ConditionalExpr) && Current->is(tok::question))
      return Current->MustBreakBefore;
    if (Current != &Token &&
        isTernaryRangeBoundary(*Current, Token.NestingLevel)) {
      break;
    }
  }
  return false;
}

bool isExpandedTemplateOpener(const FormatToken &Opener) {
  return Opener.is(TT_TemplateOpener) && Opener.MatchingParen && Opener.Next &&
         Opener.Next != Opener.MatchingParen && Opener.Next->MustBreakBefore &&
         Opener.MatchingParen->MustBreakBefore;
}

const FormatToken *findEnclosingExpandedTemplate(const AnnotatedLine &Line,
                                                 const FormatToken &Current) {
  const FormatToken *Enclosing = nullptr;
  for (const auto *Opener = Line.First; Opener && Opener != &Current;
       Opener = Opener->Next) {
    if (!isExpandedTemplateOpener(*Opener))
      continue;
    for (const auto *Inside = Opener->Next;
         Inside && Inside != Opener->MatchingParen; Inside = Inside->Next) {
      if (Inside == &Current) {
        Enclosing = Opener;
        break;
      }
    }
  }
  return Enclosing;
}

bool isArithmeticRangeBoundary(const FormatToken &Token,
                               unsigned NestingLevel) {
  if (Token.NestingLevel < NestingLevel)
    return true;
  if (Token.NestingLevel != NestingLevel)
    return false;
  return isAssignmentOperator(Token) ||
         Token.isOneOf(tok::comma, tok::semi, tok::question, tok::colon,
                       tok::kw_return, tok::kw_co_return, tok::l_brace,
                       tok::r_brace);
}

std::pair<FormatToken *, FormatToken *>
findArithmeticRange(FormatToken &Operator) {
  auto *Begin = Operator.Previous;
  while (Begin && Begin->Previous &&
         !isArithmeticRangeBoundary(*Begin->Previous, Operator.NestingLevel)) {
    Begin = Begin->Previous;
  }

  auto *End = Operator.Next;
  while (End && End->Next &&
         !isArithmeticRangeBoundary(*End->Next, Operator.NestingLevel)) {
    End = End->Next;
  }
  return {Begin, End};
}

bool isSimpleTwoOperandRange(const FormatToken &Begin, const FormatToken &End,
                             const FormatToken &Operator) {
  unsigned ArithmeticOperators = 0;
  for (const auto *Token = &Begin; Token; Token = Token->Next) {
    if (isUnsafeArithmeticToken(*Token) ||
        Token->isOneOf(tok::l_paren, tok::r_paren, tok::l_square, tok::r_square,
                       tok::l_brace, tok::r_brace, tok::comma, tok::question,
                       tok::colon, tok::semi) ||
        (Token != &Operator &&
         (Token->is(TT_BinaryOperator) || Token->is(TT_UnaryOperator)))) {
      return false;
    }
    if (isArithmeticOperator(*Token))
      ++ArithmeticOperators;
    if (Token == &End)
      break;
  }
  return ArithmeticOperators == 1;
}

void compactTwoOperandExpressions(AnnotatedLine &Line,
                                  const FormatStyle &FormatStyle) {
  if (FormatStyle.ColumnLimit == 0)
    return;

  for (auto *Operator = Line.First; Operator; Operator = Operator->Next) {
    if (!isArithmeticOperator(*Operator))
      continue;
    auto [Begin, End] = findArithmeticRange(*Operator);
    if (!Begin || !End || !isSimpleTwoOperandRange(*Begin, *End, *Operator)) {
      continue;
    }
    const auto Width = getFlatWidth(*Begin, *End);
    if (!Width || *Width > FormatStyle.ColumnLimit)
      continue;

    for (auto *Token = Begin->Next; Token; Token = Token->Next) {
      Token->MustBreakBefore = false;
      Token->CanBreakBefore = false;
      Token->NewlinesBefore = 0;
      if (Token == End)
        break;
    }
  }
}

bool isArithmeticCallParen(const AnnotatedLine &Line,
                           const FormatToken &LeftParen) {
  return isCallParen(Line, LeftParen) && LeftParen.MatchingParen &&
         LeftParen.Next && LeftParen.Next != LeftParen.MatchingParen;
}

bool expandArithmeticRange(AnnotatedLine &Line, FormatToken &Begin,
                           FormatToken &End, unsigned NestingLevel,
                           unsigned AvailableWidth,
                           const FormatStyle &FormatStyle,
                           bool ForceExpansion = false) {
  const auto Width = getFlatWidth(Begin, End);
  if (!Width || (!ForceExpansion && *Width <= AvailableWidth))
    return false;

  auto ExpandAtPrecedence = [&](auto IsOperator) {
    bool Found = false;
    for (auto *Token = &Begin; Token; Token = Token->Next) {
      if (Token->NestingLevel == NestingLevel && IsOperator(*Token)) {
        Found = true;
        break;
      }
      if (Token == &End)
        break;
    }
    if (!Found)
      return false;

    auto *SegmentBegin = &Begin;
    unsigned SegmentWidth = AvailableWidth;
    for (auto *Token = &Begin; Token; Token = Token->Next) {
      if (Token->NestingLevel == NestingLevel && IsOperator(*Token)) {
        if (SegmentBegin != Token && Token->Previous) {
          expandArithmeticRange(Line, *SegmentBegin, *Token->Previous,
                                NestingLevel, SegmentWidth, FormatStyle, false);
        }
        forceLeadingOperatorBreak(*Token);
        SegmentBegin = Token->Next;
        const unsigned PrefixWidth =
            Token->ColumnWidth +
            (Token->Next ? Token->Next->SpacesRequiredBefore : 1u);
        SegmentWidth =
            AvailableWidth > PrefixWidth ? AvailableWidth - PrefixWidth : 1;
      }
      if (Token == &End)
        break;
    }
    if (SegmentBegin)
      expandArithmeticRange(Line, *SegmentBegin, End, NestingLevel,
                            SegmentWidth, FormatStyle, false);
    return true;
  };

  if (ExpandAtPrecedence(isAdditiveOperator))
    return true;
  if (ExpandAtPrecedence(isMultiplicativeOperator))
    return true;

  if (Begin.is(tok::l_paren) && Begin.MatchingParen == &End && Begin.Next &&
      Begin.Next != &End && End.Previous) {
    if (Begin.Previous && isArithmeticOperator(*Begin.Previous))
      forceBreakBefore(Begin);
    forceBreakBefore(*Begin.Next);
    forceBreakBefore(End);
    const unsigned NestedWidth =
        AvailableWidth > FormatStyle.ContinuationIndentWidth
            ? AvailableWidth - FormatStyle.ContinuationIndentWidth
            : 1;
    expandArithmeticRange(Line, *Begin.Next, *End.Previous, NestingLevel + 1,
                          NestedWidth, FormatStyle, false);
    return true;
  }

  for (auto *Token = &Begin; Token; Token = Token->Next) {
    if (isArithmeticCallParen(Line, *Token) && Token->MatchingParen &&
        Token->MatchingParen->NestingLevel >= NestingLevel) {
      forceMultilineList(*Token);
      return true;
    }
    if (Token == &End)
      break;
  }
  return false;
}

void expandArithmeticExpression(AnnotatedLine &Line,
                                const FormatStyle &FormatStyle) {
  if (FormatStyle.ColumnLimit == 0 || Line.InPPDirective || Line.InMacroBody)
    return;

  FormatToken *ExpressionBegin = nullptr;
  bool BreakAfterAssignment = false;
  const unsigned RootNesting = Line.First->NestingLevel;
  for (auto *Token = Line.First; Token; Token = Token->Next) {
    if (Token->NestingLevel != RootNesting)
      continue;
    if (isAssignmentOperator(*Token) && Token->Next) {
      ExpressionBegin = Token->Next;
      BreakAfterAssignment = true;
    }
  }
  if (!ExpressionBegin &&
      Line.First->isOneOf(tok::kw_return, tok::kw_co_return)) {
    ExpressionBegin = Line.First->Next;
  }
  if (!ExpressionBegin)
    return;

  auto *ExpressionEnd = ExpressionBegin;
  for (auto *Token = ExpressionBegin; Token; Token = Token->Next) {
    if (Token->NestingLevel == RootNesting && Token->is(tok::semi))
      break;
    ExpressionEnd = Token;
  }
  if (!ExpressionEnd ||
      !rangeContainsArithmetic(*ExpressionBegin, *ExpressionEnd)) {
    return;
  }

  const unsigned ContinuationColumn = Line.Level * FormatStyle.IndentWidth +
                                      FormatStyle.ContinuationIndentWidth;
  const unsigned AvailableWidth =
      FormatStyle.ColumnLimit > ContinuationColumn
          ? FormatStyle.ColumnLimit - ContinuationColumn
          : 1;

  const auto FullLineWidth = getFlatWidth(*Line.First, *ExpressionEnd);
  const bool FullLineOverflows =
      FullLineWidth &&
      Line.First->OriginalColumn + *FullLineWidth > FormatStyle.ColumnLimit;
  bool IsMultiline = ExpressionBegin->NewlinesBefore > 0;
  unsigned RootOperators = 0;
  for (const auto *Token = ExpressionBegin; Token; Token = Token->Next) {
    IsMultiline |= Token->NewlinesBefore > 0;
    if (Token->NestingLevel == ExpressionBegin->NestingLevel &&
        isArithmeticOperator(*Token)) {
      ++RootOperators;
    }
    if (Token == ExpressionEnd)
      break;
  }
  const bool ForceRootExpansion =
      BreakAfterAssignment && FullLineOverflows && RootOperators > 1;

  if (IsMultiline) {
    for (auto *Token = ExpressionBegin; Token; Token = Token->Next) {
      if (isArithmeticOperator(*Token) && Token->NewlinesBefore > 0)
        forceLeadingOperatorBreak(*Token);
      if (Token == ExpressionEnd)
        break;
    }
  }

  expandArithmeticRange(Line, *ExpressionBegin, *ExpressionEnd,
                        ExpressionBegin->NestingLevel, AvailableWidth,
                        FormatStyle, ForceRootExpansion);
  if (BreakAfterAssignment && (FullLineOverflows || IsMultiline)) {
    forceBreakBefore(*ExpressionBegin);
  }
}

bool isExpandedArithmeticGroup(const FormatToken &LeftParen) {
  if (LeftParen.isNot(tok::l_paren) || !LeftParen.MatchingParen ||
      !LeftParen.Next || LeftParen.Next == LeftParen.MatchingParen ||
      hasCallCallee(LeftParen) || !LeftParen.Next->MustBreakBefore ||
      !LeftParen.MatchingParen->MustBreakBefore) {
    return false;
  }
  return rangeContainsArithmetic(*LeftParen.Next,
                                 *LeftParen.MatchingParen->Previous);
}

bool isExpandedArithmeticCall(const FormatToken &LeftParen) {
  if (LeftParen.isNot(tok::l_paren) || !LeftParen.MatchingParen ||
      !LeftParen.Next || LeftParen.Next == LeftParen.MatchingParen ||
      !LeftParen.Next->MustBreakBefore ||
      !LeftParen.MatchingParen->MustBreakBefore) {
    return false;
  }

  const auto *BeforeCall = LeftParen.getPreviousNonComment();
  while (BeforeCall &&
         BeforeCall->isOneOf(tok::identifier, tok::coloncolon, tok::period,
                             tok::arrow, TT_TemplateCloser)) {
    BeforeCall = BeforeCall->getPreviousNonComment();
  }
  return BeforeCall && isArithmeticOperator(*BeforeCall);
}

} // namespace

void customizeSyntaxLayout(AnnotatedLine &Line,
                           const FormatStyle &FormatStyle) {
  const auto *Style = getActiveExtensionStyleConst();
  if (!Style)
    return;

  if (Style->BreakConstructorDestructorSpecifiers)
    breakConstructorDestructorSpecifier(Line);

  if (Style->CompactSingleArgumentCalls) {
    for (auto *Token = Line.First; Token; Token = Token->Next) {
      if (isCallParen(Line, *Token) && hasOneSimpleArgument(*Token))
        compactSingleArgumentCall(*Token);
    }
  }

  if (Style->CompactTwoOperandExpressions)
    compactTwoOperandExpressions(Line, FormatStyle);
  if (Style->BodyDrivenLambdaExpansion)
    expandBodyDrivenLambdas(Line, FormatStyle);
  if (Style->VerticalTernaryExpressions)
    expandOverflowingTernaries(Line, FormatStyle);
  if (Style->ScopeStyleNestedTemplates)
    expandOverflowingNestedTemplates(Line, FormatStyle);
  if (Style->ProgressiveArithmeticExpansion)
    expandArithmeticExpression(Line, FormatStyle);

  if (Style->ExpandNestedAggregateBraces)
    expandNestedAggregate(Line, FormatStyle);
  if (Style->BreakRequiresExpressionBraces)
    breakRequiresExpression(Line);
  if (Style->SpaceParameterPackEllipses)
    completePackEllipsisLayout(Line);
  if (Style->SpaceAnnotationsAndFunctionPointers)
    breakFunctionPointerParameters(Line);
}

std::optional<unsigned> getSyntaxNewLineColumn(const LineState &State,
                                               const FormatStyle &FormatStyle) {
  const auto *Style = getActiveExtensionStyleConst();
  if (!Style || !State.NextToken || State.Stack.empty()) {
    return std::nullopt;
  }

  if (Style->BodyDrivenLambdaExpansion) {
    const FormatToken *LeftParen = nullptr;
    if (State.NextToken->is(tok::r_paren) && State.NextToken->MatchingParen &&
        isExpandedLambdaParen(*State.NextToken->MatchingParen)) {
      LeftParen = State.NextToken->MatchingParen;
    } else if (State.NextToken->is(tok::r_brace) &&
               State.NextToken->MatchingParen) {
      LeftParen = findExpandedLambdaParen(*State.NextToken->MatchingParen);
    } else if (State.NextToken->is(tok::l_brace) &&
               State.NextToken->MustBreakBefore) {
      LeftParen = findExpandedLambdaParen(*State.NextToken);
    } else if (State.NextToken->MustBreakBefore) {
      LeftParen = findEnclosingExpandedLambda(*State.Line, *State.NextToken);
    }
    if (LeftParen) {
      return State.FirstIndent + (LeftParen->NestingLevel + 1) *
                                     FormatStyle.ContinuationIndentWidth;
    }
  }
  if (Style->VerticalTernaryExpressions &&
      isExpandedVerticalTernaryContinuation(*State.NextToken)) {
    return State.FirstIndent + (State.NextToken->NestingLevel + 1) *
                                   FormatStyle.ContinuationIndentWidth;
  }
  if (Style->ScopeStyleNestedTemplates && State.NextToken->MustBreakBefore) {
    const FormatToken *Opener = nullptr;
    if (State.NextToken->is(TT_TemplateCloser) &&
        State.NextToken->MatchingParen &&
        isExpandedTemplateOpener(*State.NextToken->MatchingParen)) {
      Opener = State.NextToken->MatchingParen;
    } else {
      Opener = findEnclosingExpandedTemplate(*State.Line, *State.NextToken);
    }
    if (Opener) {
      return State.FirstIndent +
             (Opener->NestingLevel + 1) * FormatStyle.ContinuationIndentWidth;
    }
  }
  if (Style->BreakConstructorDestructorSpecifiers &&
      (State.NextToken->is(TT_CtorDtorDeclName) ||
       (State.NextToken->Previous &&
        State.NextToken->Previous->isOneOf(tok::kw_constexpr, tok::kw_consteval,
                                           tok::kw_virtual)))) {
    return State.FirstIndent;
  }
  if (Style->ProgressiveArithmeticExpansion) {
    if (State.NextToken->is(tok::l_paren) &&
        isExpandedArithmeticGroup(*State.NextToken)) {
      return State.FirstIndent + (State.NextToken->NestingLevel + 1) *
                                     FormatStyle.ContinuationIndentWidth;
    }
    if (State.NextToken->is(tok::r_paren) && State.NextToken->MatchingParen &&
        (isExpandedArithmeticGroup(*State.NextToken->MatchingParen) ||
         isExpandedArithmeticCall(*State.NextToken->MatchingParen))) {
      const unsigned Levels =
          isExpandedArithmeticGroup(*State.NextToken->MatchingParen)
              ? State.NextToken->MatchingParen->NestingLevel + 1
              : State.NextToken->MatchingParen->NestingLevel + 2;
      return State.FirstIndent + Levels * FormatStyle.ContinuationIndentWidth;
    }
    if (isArithmeticOperator(*State.NextToken)) {
      return State.FirstIndent + (State.NextToken->NestingLevel + 1) *
                                     FormatStyle.ContinuationIndentWidth;
    }
    if (const auto *CallParen =
            findEnclosingCallParen(*State.Line, *State.NextToken);
        CallParen && isExpandedArithmeticCall(*CallParen)) {
      return State.FirstIndent + (CallParen->NestingLevel + 2) *
                                     FormatStyle.ContinuationIndentWidth;
    }
    const auto *Previous = State.NextToken->getPreviousNonComment();
    if (Previous && Previous->is(tok::l_paren) &&
        (isExpandedArithmeticGroup(*Previous) ||
         isExpandedArithmeticCall(*Previous))) {
      return State.FirstIndent +
             (Previous->NestingLevel + 2) * FormatStyle.ContinuationIndentWidth;
    }
  }
  const auto *OpeningToken = State.Stack.back().Tok;
  if (Style->ExpandNestedAggregateBraces && OpeningToken) {
    if (State.NextToken->is(tok::l_brace)) {
      return State.FirstIndent +
             State.NextToken->NestingLevel * FormatStyle.IndentWidth;
    }
    if (State.NextToken->is(tok::r_brace) && State.NextToken->MatchingParen) {
      return State.FirstIndent + State.NextToken->MatchingParen->NestingLevel *
                                     FormatStyle.IndentWidth;
    }
    if (OpeningToken->is(tok::l_brace)) {
      return State.FirstIndent +
             (OpeningToken->NestingLevel + 1) * FormatStyle.IndentWidth;
    }
  }
  if (Style->BreakRequiresExpressionBraces &&
      State.NextToken->is(TT_RequiresExpressionLBrace)) {
    return State.FirstIndent;
  }
  if (Style->BreakRequiresExpressionBraces &&
      State.Line->Type == LT_RequiresExpression) {
    return State.FirstIndent;
  }
  return std::nullopt;
}

} // namespace clang::format::extensions
