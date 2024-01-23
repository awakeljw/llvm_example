#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetSelect.h"



#include <memory>

using namespace llvm;
int main() {
    InitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();
    LLVMContext Context;
    std::string ErrorMessage;
    Expected<std::unique_ptr<MemoryBuffer>> BitcodeBuffer = errorOrToExpected(MemoryBuffer::getFile("./sum.bc"));
    if (!BitcodeBuffer) {
        errs() << "sum.bc not found\n";
        return -1;
    }

    MemoryBufferRef BitcodeBufferRef = (**BitcodeBuffer).getMemBufferRef();
    Expected<std::unique_ptr<Module>> M = parseBitcodeFile(BitcodeBufferRef, Context);
    if (!M) {
        errs() << M.takeError() << "\n";
        return -1;
    }


    ExecutionEngine* EE(EngineBuilder(std::move(*M)).create());
    Function* SumFn = EE->FindFunctionNamed("sum");
    if (SumFn == nullptr) {
        errs() << "sumfun is null\n";
    }
    int (*Sum)(int, int) = (int (*)(int, int)) EE->getPointerToFunction(SumFn);
    outs() << "run function\n";
    int res = Sum(4, 5);
    outs() << "Sum result: " << res << "\n";
    llvm_shutdown();
    return 0;
}


