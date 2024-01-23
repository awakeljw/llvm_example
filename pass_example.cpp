#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/Threading.h"
#include "llvm-c/Core.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm-c/BitReader.h"
#include "llvm/IR/Verifier.h"

// Optimizations
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include <memory>

using namespace llvm;
int main(int argc, char** argv) {
    int n = argc > 1 ? atoi(argv[1]) : 24;
    InitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();
    LLVMStartMultithreaded();
    LLVMContext Context;
    std::string ErrorMessage;
    Expected<std::unique_ptr<MemoryBuffer>> BitcodeBuffer = errorOrToExpected(MemoryBuffer::getFile("./cfib.bc"));
    if (!BitcodeBuffer) {
        errs() << "cfib.bc not found\n";
        return -1;
    }

    MemoryBufferRef BitcodeBufferRef = (**BitcodeBuffer).getMemBufferRef();
    Expected<std::unique_ptr<Module>> M = parseBitcodeFile(BitcodeBufferRef, Context);
    if (!M) {
        errs() << M.takeError() << "\n";
        return -1;
    }

    std::unique_ptr<Module> module = std::move(*M);
    for (Module::const_iterator i = module->getFunctionList().begin(), 
        e = module->getFunctionList().end(); i != e; ++i) {
        if (!i->isDeclaration()) {
            outs() << i->getName() << " has " << i->size()
                   << " basic blocks.\n";
        }
    }

    outs() << "\n----------\n" << *module.get();
    outs() << "-----------\n";

    outs() << "function pass manager\n";
    llvm::PassManagerBuilder builder;
    llvm::legacy::PassManager mpm;
    auto fpm = std::make_unique<legacy::FunctionPassManager>(module.get());
    //fpm->add(llvm::createBasicAAWrapperPass());
    //fpm->add(llvm::createPromoteMemoryToRegisterPass());
    //fpm->add(llvm::createInstructionCombiningPass());
    //fpm->add(llvm::createReassociatePass());
    //fpm->add(llvm::createNewGVNPass());
    //fpm->add(llvm::createCFGSimplificationPass());
    unsigned optLevel = 3;
    unsigned sizeLevel = 0;
    builder.OptLevel = optLevel;
    builder.SizeLevel = sizeLevel;
    builder.Inliner = llvm::createFunctionInliningPass(
    optLevel, sizeLevel, /*DisableInlineHotCallSite=*/false);
    builder.LoopVectorize = optLevel > 1 && sizeLevel < 2;
    builder.SLPVectorize = optLevel > 1 && sizeLevel < 2;
    builder.DisableUnrollLoops = (optLevel == 0);
    builder.populateFunctionPassManager(*fpm);
    outs() << "populateModulePassManager\n";
    builder.populateModulePassManager(mpm);
    outs() << "end\n";
    // For each function in the module
    Module::iterator it;
    Module::iterator end = module.get()->end();
    for (it = module.get()->begin(); it != end; ++it) {
      // Run the FPM on this function
      outs() << it->getName() << " run optimization\n";
      fpm->run(*it);
    }
    fpm->doFinalization();
    mpm.run(*module.get());

    if (verifyModule(*module.get())) {
        return -1;
    }

    outs() << "\n----------\n" << *module.get();
    outs() << "-----------\n";

    llvm::EngineBuilder factory(std::move(module));
    factory.setEngineKind(llvm::EngineKind::JIT);
    ExecutionEngine* EE(factory.create());

    Function* fibfunc = EE->FindFunctionNamed("fibonacci");
    if (fibfunc == nullptr) {
        errs() << "fibonacci func is null\n";
        return -1;
    }
    //typedef int (*fibType)(int);
    //EE->finalizeObject();
    //fibType ffib = reinterpret_cast<fibType>(EE->getPointerToFunction(fibfunc));
    //fibType ffib = reinterpret_cast<fibType>( EE->getFunctionAddress("fibonacci"));
    //outs() << "run function\n";
    //int res = ffib(n);
    //outs() << "fibonacci result: " << res << "\n";
    std::vector<GenericValue> Args(1);
    Args[0].IntVal = APInt(32, n);
    GenericValue res = EE->runFunction(fibfunc, Args);
    outs() << "fibonacci result: " << res.IntVal << "\n";
    delete EE;
    llvm_shutdown();
    return 0;
}


