// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TOOLS_TIDY_MODULE_BRACED_MEMBER_INIT_CHECK_H_
#define TOOLS_TIDY_MODULE_BRACED_MEMBER_INIT_CHECK_H_

#include "clang-tidy/ClangTidyCheck.h"

namespace house {

/*!
 * \brief house-braced-member-init: an initializer of a constructor's member
 *        initializer list is written with braces, `value_{value}`, not with
 *        parentheses, `value_(value)`.
 *
 * The style's braced initialization, which no check of clang-tidy's own holds a
 * member initializer to: a narrowing conversion is an error in braces and
 * passes in parentheses. Members, bases and a delegating constructor alike.
 *
 * The fix-it swaps the parentheses for braces where that keeps the meaning, and
 * only there:
 *
 * - Not reported at all where braces would choose a `std::initializer_list`
 *   constructor that the parentheses do not, as Google writes parentheses
 *   there; empty parentheses are reported, as `{}` value-initializes as `()`
 *   does.
 * - Reported without a fix-it where an argument converts narrowing: braces
 *   would make it an error, and the conversion wants to be written out.
 * - Reported without a fix-it where the type is a template's parameter: what
 *   the braces choose depends on the instantiation.
 */
class BracedMemberInitCheck : public clang::tidy::ClangTidyCheck {
 public:
  using ClangTidyCheck::ClangTidyCheck;

  bool isLanguageVersionSupported(
      clang::LangOptions const& lang_opts) const override {
    return lang_opts.CPlusPlus11;
  }
  void registerMatchers(clang::ast_matchers::MatchFinder* finder) override;
  void check(
      clang::ast_matchers::MatchFinder::MatchResult const& result) override;
};

}  // namespace house

#endif  // TOOLS_TIDY_MODULE_BRACED_MEMBER_INIT_CHECK_H_
