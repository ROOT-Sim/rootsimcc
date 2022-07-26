/**
 * @file incremental.cpp
 *
 * @brief LLVM plugin to instrument memory writes
 *
 * This is the LLVM plugin which is able to
 * instrument memory writes of arbitrary code.
 *
 * SPDX-FileCopyrightText: 2008-2022 Andrea Piccione <piccione@diag.uniroma1.it>
 */
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Casting.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/Cloning.h"

#define LLVM_USE_NEW_PASSMANAGER 0
#define AVOID_INLINE_MEMTRACE 0

using namespace llvm;

namespace {
	enum instrumentation_stats {
		TRACED_STORE,
		TRACED_MEMSET,
		TRACED_MEMCPY,
		TRACED_CALL,
		TRACED_ATOMIC,
		TRACED_UNKNOWN,
		INSTRUMENTATION_STATS_COUNT
	};

#if LLVM_USE_NEW_PASSMANAGER
class MemInstrPass: public PassInfoMixin<MemInstrPass> {
	FunctionAnalysisManager *fa_manager = nullptr;
public:
	PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
		fa_manager = &AM.getResult<FunctionAnalysisManagerModuleProxy>(M)
				.getManager();
		ProcessModule(M);
		return PreservedAnalyses::none();
	}
#else
class MemInstrPass: public ModulePass {
	TargetLibraryInfoWrapperPass *lib_info_pass = nullptr;
public:
	MemInstrPass() : ModulePass(ID) {}

	virtual void getAnalysisUsage(AnalysisUsage &AU) const
	{
		AU.addRequired<TargetLibraryInfoWrapperPass>();
	}

	bool runOnModule(Module &M)
	{
		lib_info_pass = &getAnalysis<TargetLibraryInfoWrapperPass>();
		ProcessModule(M);
		return true;
	}

#endif
private:
	static char ID;
	unsigned stats[INSTRUMENTATION_STATS_COUNT] = {0};

	void ProcessModule(Module &M)
	{
		errs() << "Instrumenting module " << raw_ostream::CYAN <<
				M.getName() << "\n";
		errs().resetColor();

		FunctionCallee wfnc = InitMemtraceFunction(M, "__write_mem");

		for (Function &F : M) {
			if (isSystemSide(F) || F.isDeclaration())
				continue;

			for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I)
				if (I->mayWriteToMemory())
					InstrumentWriteInstruction(M, &*I, wfnc);
		}

		errs() << "Instrumented " << raw_ostream::GREEN << stats[TRACED_STORE] << " stores\n";
		errs().resetColor();
		errs() << "Instrumented " << raw_ostream::GREEN << stats[TRACED_MEMSET] << " memsets\n";
		errs().resetColor();
		errs() << "Instrumented " << raw_ostream::GREEN << stats[TRACED_MEMCPY] << " memcpys\n";
		errs().resetColor();
		errs() << "Encountered " << raw_ostream::CYAN << stats[TRACED_CALL] << " calls\n";
		errs().resetColor();
		errs() << "Encountered " << raw_ostream::RED << stats[TRACED_ATOMIC] << " atomics\n";
		errs().resetColor();
		errs() << "Encountered " << raw_ostream::RED << stats[TRACED_UNKNOWN] << " unknown instructions\n";
		errs().resetColor();
	}

	bool isSystemSide(Function &F)
	{

		if (F.getIntrinsicID() || F.doesNotReturn())
			return true;

		enum llvm::LibFunc LLF;
#if LLVM_USE_NEW_PASSMANAGER
		return fa_manager->getResult<TargetLibraryAnalysis>(F).getLibFunc(F, LLF);
#elif LLVM_VERSION_MAJOR >= 10
		return lib_info_pass->getTLI(F).getLibFunc(F, LLF);
#else
		return lib_info_pass->getTLI().getLibFunc(F, LLF);
#endif
	}

	FunctionCallee InitMemtraceFunction(Module &M, const char *memtrace_name)
	{
		LLVMContext &ctx = M.getContext();

		Type *MemtraceArgs[] = {
			Type::getInt8PtrTy(ctx),
			IntegerType::get(M.getContext(), sizeof(size_t) * CHAR_BIT)
		};

		FunctionType *Fty = FunctionType::get(
			Type::getVoidTy(M.getContext()),
			MemtraceArgs,
			false
		);
#ifdef AVOID_INLINE_MEMTRACE
		AttributeList al = AttributeList();
		al = al.addAttribute(ctx, 0, Attribute::NoInline);
		return M.getOrInsertFunction(memtrace_name, Fty, al);
#else
		return M.getOrInsertFunction(memtrace_name, Fty);
#endif
	}

	void InstrumentWriteInstruction(Module &M, Instruction *TI,
					FunctionCallee &memtrace_fnc)
	{
		Value *args[2];

		if (StoreInst *SI = dyn_cast<StoreInst>(TI)) {
			Value *V = SI->getPointerOperand();
			PointerType *pType = cast<PointerType>(V->getType());
			uint64_t storeSize = M.getDataLayout().getTypeStoreSize(
					pType->getElementType());

			args[0] = CastInst::CreatePointerBitCastOrAddrSpaceCast(
				V, memtrace_fnc.getFunctionType()->getParamType(0), "", TI);
			args[1] = ConstantInt::get(IntegerType::get(M.getContext(),
				sizeof(size_t) * CHAR_BIT), storeSize);
			++stats[TRACED_STORE];
		} else if (MemSetInst *MSI = dyn_cast<MemSetInst>(TI)) {
			args[0] = MSI->getRawDest();
			args[1] = MSI->getLength();
			++stats[TRACED_MEMSET];
		} else if (MemCpyInst *MCI = dyn_cast<MemCpyInst>(TI)) {
			args[0] = MCI->getRawDest();
			args[1] = MCI->getLength();
			++stats[TRACED_MEMCPY];
		} else {
			 if (isa<CallBase>(TI)) {
				 ++stats[TRACED_CALL];
			} else if (TI->isAtomic()) {
				errs() << "Encountered an atomic non-store instruction in function "
					<< TI->getParent()->getParent()->getName() << "\n";
				++stats[TRACED_ATOMIC];
			} else {
				errs() << "Encountered an unknown memory writing instruction in function "
					<< TI->getParent()->getParent()->getName() << "\n";
				++stats[TRACED_UNKNOWN];
			}
			return;
		}

		CallInst *c = CallInst::Create(memtrace_fnc, args, "", TI);
		c->setDebugLoc(TI->getDebugLoc());
	}
};
}

#if LLVM_USE_NEW_PASSMANAGER

static void newPassManagerLoadPass(ModulePassManager &MPM,
                PassBuilder::OptimizationLevel Level) {
	MPM.addPass(MemInstrPass());
}

void memInstrPluginRegister(PassBuilder &PB)
{
	PB.registerOptimizerLastEPCallback(newPassManagerLoadPass);
}

llvm::PassPluginLibraryInfo memInstrPluginInfoGet(void)
{
	return {LLVM_PLUGIN_API_VERSION, "MemInstr plugin", "0.0.1",
		memInstrPluginRegister};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo()
{
	return memInstrPluginInfoGet();
}

#else

char MemInstrPass::ID = 0;

static void loadPass(const PassManagerBuilder &Builder,
		llvm::legacy::PassManagerBase &PM)
{
	(void)Builder;
	PM.add(new MemInstrPass());
}

static RegisterStandardPasses clangtoolLoader_Ox(
	PassManagerBuilder::EP_ModuleOptimizerEarly, loadPass);

static RegisterStandardPasses clangtoolLoader_O0(
	PassManagerBuilder::EP_EnabledOnOptLevel0, loadPass);

#endif
