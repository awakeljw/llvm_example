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
        errs() << "fibonacci.bc not found\n";
        return -1;
    }

    MemoryBufferRef BitcodeBufferRef = (**BitcodeBuffer).getMemBufferRef();
    Expected<std::unique_ptr<Module>> M = parseBitcodeFile(BitcodeBufferRef, Context);
    if (!M) {
        errs() << M.takeError() << "\n";
        return -1;
    }
    for (Module::const_iterator i = M.get()->getFunctionList().begin(), 
        e = M.get()->getFunctionList().end(); i != e; ++i) {
        if (!i->isDeclaration()) {
            outs() << i->getName() << " has " << i->size()
                   << " basic blocks.\n";
        }
    }

    llvm::EngineBuilder factory(std::move(*M));
    factory.setEngineKind(llvm::EngineKind::Either);
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


