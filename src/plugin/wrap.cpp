/**
 * @file plugin/wrap.cpp
 *
 * @brief LLVM compiler plugin support for rootsim-cc
 *
 * This is the header of the LLVM plugin used to manipulate model's code.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <plugin/wrap.h>

#define ROOTSIM_WRAPPER_PASS_SUFFIX "_instr"

using namespace llvm;

Function *RootsimWrapperPass::CloneFunctionStub(Function &F, const std::string &new_name) {
	Function *NewF = Function::Create(cast<FunctionType>(F.getValueType()), F.getLinkage(),
									  F.getAddressSpace(), new_name, F.getParent());
	NewF->copyAttributesFrom(&F);
	return NewF;
}

void RootsimWrapperPass::CloneFunctionIntoAndMap(Function *NewF, const Function &F, ValueToValueMapTy &VMap,
												 const char *suffix) {
	Function::arg_iterator DestI = NewF->arg_begin();
	for (const Argument &I: F.args()) {
		DestI->setName(I.getName());
		VMap[&I] = DestI++;
	}

	SmallVector<ReturnInst *, 8> Returns;
#if LLVM_VERSION_MAJOR >= 14
	// fixme: check the CloneFunctionChangeType value
	CloneFunctionInto(NewF, &F, VMap, llvm::CloneFunctionChangeType::GlobalChanges, Returns, suffix);
#else
	CloneFunctionInto(NewF, &F, VMap, true, Returns, suffix);
#endif
	for (const Argument &I: F.args()) {
		VMap.erase(&I);
	}
	// XXX: solves an LLVM bug but removes debug info from clones
	NewF->setSubprogram(nullptr);
}

#ifdef LLVM_USE_NEW_PASS_MANAGER

PreservedAnalyses RootsimWrapperPass::run(Module &M, ModuleAnalysisManager &AM) {
	fa_manager = &AM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();
	processModule(M);
	return PreservedAnalyses::none();
}

#else

void RootsimWrapperPass::getAnalysisUsage(AnalysisUsage &AU) const override {
	AU.addRequired<TargetLibraryInfoWrapperPass>();
}

bool RootsimWrapperPass::runOnModule(Module &M) override {
	lib_info_pass = &getAnalysis<TargetLibraryInfoWrapperPass>();
	processModule(M);
	return true;
}

#endif

void RootsimWrapperPass::processModule(Module &M) {
#ifndef NDEBUG
	errs() << "Instrumenting module " << raw_ostream::CYAN << M.getName() << "\n";
	errs().resetColor();
#endif

	ValueToValueMapTy VMap;
	std::vector<std::reference_wrapper<Function>> F_vec;
	for (Function &F: M) {
		if (isSystemSide(F) || isToIgnore(F)) {
#ifndef NDEBUG
			errs() << "Ignoring function " << F.getName() << "\n";
#endif
		} else {
#ifndef NDEBUG
			errs() << "Found function " << F.getName() << "\n";
#endif
			F_vec.emplace_back(F);
		}
	}

	for (Function &F: F_vec) {
		const std::string *r = isToReplace(F);
		if (r != nullptr) {
			VMap[&F] = CloneFunctionStub(F, *r);
		} else {
			VMap[&F] = CloneFunctionStub(F, F.getName().str() + ROOTSIM_WRAPPER_PASS_SUFFIX);
		}
	}

	for (Function &F: F_vec) {
		if (F.isDeclaration())
			continue;

		auto *Cloned = cast<Function>(VMap[&F]);
		if (Cloned == nullptr)
			continue;
#ifndef NDEBUG
		errs() << "Processing " << Cloned->getName() << "\n";
#endif
		CloneFunctionIntoAndMap(Cloned, F, VMap, ROOTSIM_WRAPPER_PASS_SUFFIX);
	}
}

bool RootsimWrapperPass::isToIgnore(Function &F) {
	std::string s = F.getName().str();
	return to_ignore.find(s) != to_ignore.end();
}

const std::string *RootsimWrapperPass::isToReplace(Function &F) {
	std::string s = F.getName().str();
	auto r = to_replace.find(s);
	return r == to_replace.end() ? nullptr : &r->second;
}

bool RootsimWrapperPass::isSystemSide(Function &F) {

	if (F.getIntrinsicID() || F.doesNotReturn())
		return true;

	enum llvm::LibFunc LLF;
#ifdef LLVM_USE_NEW_PASS_MANAGER
	return fa_manager->getResult<TargetLibraryAnalysis>(F).getLibFunc(F, LLF);
#elif LLVM_VERSION_MAJOR >= 10
	return lib_info_pass->getTLI(F).getLibFunc(F, LLF);
#else
	return lib_info_pass->getTLI().getLibFunc(F, LLF);
#endif
}

#ifdef LLVM_USE_NEW_PASS_MANAGER

static void newPassManagerLoadPass(ModulePassManager &MPM, OptimizationLevel Level) {
	(void) Level;
	MPM.addPass(RootsimWrapperPass());
}

static void rootsimWrapperPassRegister(PassBuilder &PB) {
	PB.registerOptimizerLastEPCallback(newPassManagerLoadPass);
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
	return {LLVM_PLUGIN_API_VERSION, "RootsimWrapperPass plugin", "0.0.1", rootsimWrapperPassRegister};
}

#else

char RootsimWrapperPass::ID = 0;

static void rootsimWrapperLoadPass(const PassManagerBuilder &Builder, llvm::legacy::PassManagerBase &PM)
{
	(void) Builder;
	PM.add(new RootsimWrapperPass());
}

static RegisterStandardPasses clangtoolLoader_Ox(
		PassManagerBuilder::EP_ModuleOptimizerEarly, rootsimWrapperLoadPass);

static RegisterStandardPasses clangtoolLoader_O0(
		PassManagerBuilder::EP_EnabledOnOptLevel0, rootsimWrapperLoadPass);

#endif

const std::unordered_map<std::string, std::string> RootsimWrapperPass::to_replace = {{"malloc", "rs_malloc"}, {"free", "rs_free"}};
const std::unordered_set<std::string> RootsimWrapperPass::to_ignore = {"rs_malloc", "rs_free", "ScheduleNewEvent", "Expent", "Random"};
