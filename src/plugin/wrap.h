/**
 * @file plugin/wrap.h
 *
 * @brief LLVM compiler plugin support for rootsim-cc
 *
 * This is the header of the LLVM plugin used to manipulate model's code.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Type.h>

#ifndef LLVM_USE_NEW_PASS_MANAGER
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Pass.h>
#include <llvm/Passes/PassBuilder.h>
#else

#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>

#endif

#include <llvm/Support/Casting.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/Utils/Cloning.h>

#include <unordered_map>
#include <unordered_set>

using namespace llvm;

namespace {

	class RootsimWrapperPass :

#ifdef LLVM_USE_NEW_PASS_MANAGER
			public PassInfoMixin<RootsimWrapperPass> {
		FunctionAnalysisManager *fa_manager = nullptr;
	public:
		PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
#else
			public ModulePass {
		TargetLibraryInfoWrapperPass *lib_info_pass = nullptr;
	public:
		RootsimWrapperPass() : ModulePass(ID) {}

		void getAnalysisUsage(AnalysisUsage &AU) const override;

		bool runOnModule(Module &M) override;
#endif

		void processModule(Module &M);

	private:
		static char ID;
		static const std::unordered_map<std::string, std::string> to_replace;
		static const std::unordered_set<std::string> to_ignore;

		static bool isToIgnore(Function &F);

		static const std::string *isToReplace(Function &F);

		static Function *CloneFunctionStub(Function &F, const std::string &new_name);

		static void
		CloneFunctionIntoAndMap(Function *NewF, const Function &F, ValueToValueMapTy &VMap, const char *suffix);

		bool isSystemSide(Function &F);
	};
}

#ifdef LLVM_USE_NEW_PASS_MANAGER

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo();

#endif
