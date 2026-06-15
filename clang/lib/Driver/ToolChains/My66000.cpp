//===--- My66000.cpp - My66000 ToolChain Implementations --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "My66000.h"
#include "clang/Driver/CommonArgs.h"
#include "clang/Driver/Compilation.h"
#include "clang/Options/Options.h"
#include "llvm/Option/ArgList.h"
#include <cstdlib> // ::getenv

using namespace clang::driver;
using namespace clang::driver::toolchains;
using namespace clang::driver::tools;
using namespace clang;
using namespace llvm::opt;

/// My66000 Tools

void tools::My66000::Assembler::ConstructJob(Compilation &C, const JobAction &JA,
                                           const InputInfo &Output,
                                           const InputInfoList &Inputs,
                                           const ArgList &Args,
                                           const char *LinkingOutput) const {
  claimNoWarnArgs(Args);
  ArgStringList CmdArgs;

  CmdArgs.push_back("-o");
  CmdArgs.push_back(Output.getFilename());

  if (Args.hasArg(options::OPT_msmall))
    CmdArgs.push_back("-mcmodel=small");
  else if (Args.hasArg(options::OPT_mlarge))
    CmdArgs.push_back("-mcmodel=large");
  else
    CmdArgs.push_back("-mcmodel=tiny");
/*
  if (Arg *A = Args.getLastArg(clang::driver::options::OPT_mcmodel_EQ)) {
    StringRef CM = A->getValue();
    CmdArgs.push_back(Args.MakeArgString("-mcmodel=" + CM));
  } else
    CmdArgs.push_back("-mcmodel=tiny");
*/
  Args.AddAllArgValues(CmdArgs, options::OPT_Wa_COMMA, options::OPT_Xassembler);

  for (const auto &II : Inputs)
    CmdArgs.push_back(II.getFilename());

  const char *Exec = Args.MakeArgString(getToolChain().
		     GetProgramPath("my66000-unknown-elf-as"));
  C.addCommand(std::make_unique<Command>(JA, *this, ResponseFileSupport::None(),
                                         Exec, CmdArgs, Inputs, Output));
}

void tools::My66000::Linker::ConstructJob(Compilation &C, const JobAction &JA,
                                        const InputInfo &Output,
                                        const InputInfoList &Inputs,
                                        const ArgList &Args,
                                        const char *LinkingOutput) const {
  ArgStringList CmdArgs;

  assert((Output.isFilename() || Output.isNothing()) && "Invalid output.");
  if (Output.isFilename()) {
    CmdArgs.push_back("-o");
    CmdArgs.push_back(Output.getFilename());
  }

  CmdArgs.push_back("-L/usr/local/bin/my66000-unknown-elf/lib");

  AddLinkerInputs(getToolChain(), Inputs, Args, CmdArgs, JA);

  const char *Exec = Args.MakeArgString(getToolChain().
		     GetProgramPath("my66000-unknown-elf-ld"));
  C.addCommand(std::make_unique<Command>(JA, *this, ResponseFileSupport::None(),
                                         Exec, CmdArgs, Inputs, Output));
}

/// My66000 tool chain
My66000ToolChain::My66000ToolChain(const Driver &D, const llvm::Triple &Triple,
                               const ArgList &Args)
    : ToolChain(D, Triple, Args) {
  // ProgramPaths are found via 'PATH' environment variable.
}

Tool *My66000ToolChain::buildAssembler() const {
  return new tools::My66000::Assembler(*this);
}

Tool *My66000ToolChain::buildLinker() const {
  return new tools::My66000::Linker(*this);
}

bool My66000ToolChain::isPICDefault() const { return true; }

bool My66000ToolChain::isPIEDefault(const llvm::opt::ArgList &Args) const {
  return false;
}

bool My66000ToolChain::isPICDefaultForced() const { return false; }

bool My66000ToolChain::SupportsProfiling() const { return false; }

bool My66000ToolChain::hasBlocksRuntime() const { return false; }

void My66000ToolChain::AddClangSystemIncludeArgs(const ArgList &DriverArgs,
                                               ArgStringList &CC1Args) const {
  if (DriverArgs.hasArg(options::OPT_nostdinc) ||
      DriverArgs.hasArg(options::OPT_nostdlibinc))
    return;
  addSystemInclude(DriverArgs, CC1Args, "/usr/local/my66000-unknown-elf/include");
}

void My66000ToolChain::addClangTargetOptions(const ArgList &DriverArgs,
                                           ArgStringList &CC1Args,
                                           Action::OffloadKind) const {
  CC1Args.push_back("-nostdsysteminc");
}

void My66000ToolChain::AddClangCXXStdlibIncludeArgs(
    const ArgList &DriverArgs, ArgStringList &CC1Args) const {
  if (DriverArgs.hasArg(options::OPT_nostdinc) ||
      DriverArgs.hasArg(options::OPT_nostdlibinc) ||
      DriverArgs.hasArg(options::OPT_nostdincxx))
    return;
}

void My66000ToolChain::AddCXXStdlibLibArgs(const ArgList &Args,
                                         ArgStringList &CmdArgs) const {
  // FIXME
}
