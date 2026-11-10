// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

#include "braced_member_init_check.h"

#include <optional>
#include <string>

#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Lexer.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APSInt.h"

namespace house {
namespace {

using clang::ASTContext;
using clang::CXXConstructorDecl;
using clang::CXXCtorInitializer;
using clang::CXXRecordDecl;
using clang::Expr;
using clang::QualType;

// What braces in place of the parentheses would do.
enum class Braces {
  kSame,       // the same: reported with a fix-it
  kNarrowing,  // an argument narrows, an error in braces: reported without one
  kDependent,  // depends on the instantiation: reported without one
  kInitializer,  // chooses a std::initializer_list constructor: not reported
};

// Whether a parameter of type `type` is a std::initializer_list, as written in
// a class or in a class template.
bool IsInitializerList(QualType type) {
  type = type.getNonReferenceType().getUnqualifiedType();
  clang::NamedDecl const* decl = nullptr;
  if (auto const* record = type->getAsCXXRecordDecl()) {
    decl = record;
  } else if (auto const* specialization =
                 type->getAs<clang::TemplateSpecializationType>()) {
    decl = specialization->getTemplateName().getAsTemplateDecl();
  }
  return decl != nullptr && decl->getName() == "initializer_list" &&
         decl->isInStdNamespace();
}

// Whether the class has a constructor, or a constructor template, whose first
// parameter is a std::initializer_list and whose others have a default: one
// that braces prefer.
bool HasInitializerListConstructor(CXXRecordDecl const& record) {
  for (clang::Decl const* member : record.decls()) {
    auto const* constructor = llvm::dyn_cast<CXXConstructorDecl>(member);
    if (auto const* function_template =
            llvm::dyn_cast<clang::FunctionTemplateDecl>(member)) {
      constructor = llvm::dyn_cast<CXXConstructorDecl>(
          function_template->getTemplatedDecl());
    }
    if (constructor == nullptr || constructor->getNumParams() == 0 ||
        !IsInitializerList(constructor->getParamDecl(0)->getType())) {
      continue;
    }
    if (constructor->getNumParams() == 1 ||
        constructor->getParamDecl(1)->hasDefaultArg()) {
      return true;
    }
  }
  return false;
}

// Whether an integer constant fits an integer type of `width` bits and
// `is_signed`.
bool Fits(llvm::APSInt const& value, unsigned width, bool is_signed) {
  llvm::APSInt converted{value.extOrTrunc(width)};
  converted.setIsSigned(is_signed);
  return llvm::APSInt::isSameValue(converted, value);
}

// Whether `argument` converts narrowing to `target` ([dcl.init.list]): an error
// in braces. Where a type depends on a template's parameter, it cannot be
// known, and is taken as not: the instantiation that narrows fails to compile,
// which does not go unnoticed.
bool IsNarrowing(Expr const& argument, QualType target, ASTContext& context) {
  if (target->isDependentType() || argument.isTypeDependent() ||
      argument.isValueDependent()) {
    return false;
  }
  Expr const* source_expr = argument.IgnoreParenImpCasts();
  QualType const source{
      source_expr->getType().getCanonicalType().getUnqualifiedType()};
  target = target.getNonReferenceType().getCanonicalType().getUnqualifiedType();
  if (source == target) {
    return false;
  }
  bool const target_integer = target->isIntegralOrUnscopedEnumerationType();
  bool const source_integer = source->isIntegralOrUnscopedEnumerationType();
  // A pointer to bool narrows since P1957, which compilers apply to C++11 as
  // well.
  if (target->isBooleanType() &&
      (source->isPointerType() || source->isMemberPointerType())) {
    return true;
  }
  if (target_integer && source->isRealFloatingType()) {
    return true;
  }
  std::optional<llvm::APSInt> const constant{
      source_expr->getIntegerConstantExpr(context)};
  if (target_integer && source_integer) {
    unsigned const target_width = context.getIntWidth(target);
    unsigned const source_width = context.getIntWidth(source);
    bool const target_signed = target->isSignedIntegerOrEnumerationType();
    bool const source_signed = source->isSignedIntegerOrEnumerationType();
    bool const holds_all =
        (target_signed == source_signed && target_width >= source_width) ||
        (target_signed && !source_signed && target_width > source_width);
    return !holds_all &&
           !(constant && Fits(*constant, target_width, target_signed));
  }
  if (target->isRealFloatingType() && source_integer) {
    if (!constant) {
      return true;
    }
    llvm::APFloat value{context.getFloatTypeSemantics(target)};
    return value.convertFromAPInt(*constant, constant->isSigned(),
                                  llvm::APFloat::rmNearestTiesToEven) !=
           llvm::APFloat::opOK;
  }
  if (target->isRealFloatingType() && source->isRealFloatingType()) {
    // A constant whose value the narrower type holds does not narrow; an exact
    // check of the value is left out, and a constant taken as fitting.
    return context.getFloatingTypeOrder(target, source) < 0 &&
           !source_expr->isCXX11ConstantExpr(context);
  }
  return false;
}

// What braces would do in place of the parentheses of `init`, whose initializee
// has type `type`.
Braces Classify(CXXCtorInitializer const& init, QualType type,
                ASTContext& context) {
  if (type->isDependentType()) {
    return Braces::kDependent;
  }
  Expr const* expr = init.getInit();
  if (auto const* cleanups = llvm::dyn_cast<clang::ExprWithCleanups>(expr)) {
    expr = cleanups->getSubExpr();
  }
  if (auto const* bind = llvm::dyn_cast<clang::CXXBindTemporaryExpr>(expr)) {
    expr = bind->getSubExpr();
  }

  llvm::SmallVector<Expr const*, 4> arguments;
  llvm::SmallVector<QualType, 4> targets;
  if (auto const* construct = llvm::dyn_cast<clang::CXXConstructExpr>(expr)) {
    CXXConstructorDecl const* constructor = construct->getConstructor();
    for (unsigned i = 0; i < construct->getNumArgs(); ++i) {
      if (!llvm::isa<clang::CXXDefaultArgExpr>(construct->getArg(i))) {
        arguments.push_back(construct->getArg(i));
        targets.push_back(constructor->getParamDecl(i)->getType());
      }
    }
  } else if (auto const* list = llvm::dyn_cast<clang::ParenListExpr>(expr)) {
    // In a template, as written: no constructor chosen yet.
    for (Expr const* argument : list->exprs()) {
      arguments.push_back(argument);
      targets.push_back(type);
    }
  } else if (auto const* paren_list =
                 llvm::dyn_cast<clang::CXXParenListInitExpr>(expr)) {
    // C++20's aggregate in parentheses, which braces initialize the same, but
    // for narrowing.
    for (Expr const* argument : paren_list->getInitExprs()) {
      arguments.push_back(argument);
      targets.push_back(argument->getType());
    }
  } else if (!llvm::isa<clang::ImplicitValueInitExpr>(expr)) {
    arguments.push_back(expr);
    targets.push_back(type);
  }

  if (auto const* record = type->getAsCXXRecordDecl()) {
    if (!arguments.empty()) {
      if (!record->hasDefinition()) {
        return Braces::kDependent;
      }
      if (HasInitializerListConstructor(*record->getDefinition())) {
        return Braces::kInitializer;
      }
    }
  }
  for (unsigned i = 0; i < arguments.size(); ++i) {
    if (IsNarrowing(*arguments[i], targets[i], context)) {
      return Braces::kNarrowing;
    }
  }
  return Braces::kSame;
}

// The name a diagnostic gives the initializee: the member's, or the type's of a
// base or of a delegating constructor.
std::string NameOf(CXXCtorInitializer const& init,
                   clang::PrintingPolicy const& policy) {
  if (clang::FieldDecl const* member = init.getAnyMember()) {
    return member->getNameAsString();
  }
  return init.getTypeSourceInfo()->getType().getAsString(policy);
}

}  // namespace

void BracedMemberInitCheck::registerMatchers(
    clang::ast_matchers::MatchFinder* finder) {
  using clang::ast_matchers::cxxConstructorDecl;
  using clang::ast_matchers::cxxCtorInitializer;
  using clang::ast_matchers::forEachConstructorInitializer;
  using clang::ast_matchers::isDefinition;
  using clang::ast_matchers::isImplicit;
  using clang::ast_matchers::isInstantiated;
  using clang::ast_matchers::isWritten;
  using clang::ast_matchers::unless;
  // A template as written, not each of its instantiations: an initializer is
  // reported once, at the place it is written, and a template that no file
  // instantiates is checked as well.
  finder->addMatcher(
      cxxConstructorDecl(isDefinition(), unless(isImplicit()),
                         unless(isInstantiated()),
                         forEachConstructorInitializer(
                             cxxCtorInitializer(isWritten()).bind("init"))),
      this);
}

void BracedMemberInitCheck::check(
    clang::ast_matchers::MatchFinder::MatchResult const& result) {
  auto const* init = result.Nodes.getNodeAs<CXXCtorInitializer>("init");
  clang::SourceManager const& sources = *result.SourceManager;
  clang::SourceLocation const left = init->getLParenLoc();
  clang::SourceLocation const right = init->getRParenLoc();
  // Written in a macro, the parentheses are the macro's, which the fix-it must
  // not change.
  if (left.isInvalid() || right.isInvalid() || left.isMacroID() ||
      right.isMacroID()) {
    return;
  }
  // A braced initializer has its braces where the parentheses would be.
  clang::CharSourceRange const left_range{
      clang::CharSourceRange::getCharRange(left, left.getLocWithOffset(1))};
  if (clang::Lexer::getSourceText(left_range, sources, getLangOpts()) != "(") {
    return;
  }

  QualType const type{init->getAnyMember() != nullptr
                          ? init->getAnyMember()->getType()
                          : init->getTypeSourceInfo()->getType()};
  Braces const braces = Classify(*init, type, *result.Context);
  if (braces == Braces::kInitializer) {
    return;
  }
  std::string const name{NameOf(*init, result.Context->getPrintingPolicy())};
  switch (braces) {
    case Braces::kSame:
      diag(left, "initialize '%0' with braces, not parentheses")
          << name << clang::FixItHint::CreateReplacement(left_range, "{")
          << clang::FixItHint::CreateReplacement(
                 clang::CharSourceRange::getCharRange(
                     right, right.getLocWithOffset(1)),
                 "}");
      break;
    case Braces::kNarrowing:
      diag(left,
           "initialize '%0' with braces, not parentheses; an argument converts "
           "narrowing, which "
           "braces make an error: convert it explicitly")
          << name;
      break;
    case Braces::kDependent:
      diag(left,
           "initialize '%0' with braces, not parentheses; no fix-it, as what "
           "braces choose "
           "depends on the template's arguments")
          << name;
      break;
    case Braces::kInitializer:
      break;
  }
}

}  // namespace house
