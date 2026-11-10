// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The module house: checks of the rules every project made from the template
// keeps, which clang-tidy's own checks do not. tools/lib/tidy_module.sh builds it,
// and tools/tidy.sh loads it with --load; .clang-tidy turns its checks on as
// house-*.

#include "braced_member_init_check.h"
#include "clang-tidy/ClangTidyModule.h"

namespace house {
namespace {

class HouseModule : public clang::tidy::ClangTidyModule {
 public:
  void addCheckFactories(
      clang::tidy::ClangTidyCheckFactories& factories) override {
    factories.registerCheck<BracedMemberInitCheck>("house-braced-member-init");
  }
};

}  // namespace
}  // namespace house

// Registered when clang-tidy loads the module, by the constructor of this
// object.
static clang::tidy::ClangTidyModuleRegistry::Add<house::HouseModule> const
    kModule{
        "house",
        "The checks of the rules every project made from the template keeps."};
