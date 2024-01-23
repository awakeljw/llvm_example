#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Mangler.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"

#include <memory>

using namespace llvm;
using namespace std;
using namespace llvm::object;

std::string mangleName(Module module, const std::string &name) {
  std::string mangledName;
  raw_string_ostream mangledNameStream(mangledName);
  Mangler::getNameWithPrefix(mangledNameStream, name, module.getDataLayout());
  return mangledName;
}

std::string getMangledName(Module &M, const std::string &name) {
  std::string MangledName;
  {
    raw_string_ostream MangledNameStream(MangledName);
    Mangler::getNameWithPrefix(MangledNameStream, name, M.getDataLayout());
  }
  return MangledName;
}

int main() {
  InitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();
  LLVMContext Context;
  std::string ErrorMessage;
  Expected<std::unique_ptr<MemoryBuffer>> BitcodeBuffer =
      errorOrToExpected(MemoryBuffer::getFile("./sum.bc"));
  if (!BitcodeBuffer) {
    errs() << "sum.bc not found\n";
    return -1;
  }

  MemoryBufferRef BitcodeBufferRef = (**BitcodeBuffer).getMemBufferRef();
  Expected<std::unique_ptr<Module>> M =
      parseBitcodeFile(BitcodeBufferRef, Context);
  if (!M) {
    errs() << M.takeError() << "\n";
    return -1;
  }

  size_t len = 5;
  char *Buf = static_cast<char *>(std::malloc(len));
  ItaniumPartialDemangler Demangler;
  auto module = std::move(*M);
  for (auto &func : *module) {
    auto func_name = std::string(func.getName());
    auto mangled_name = getMangledName(*module, func_name);
    outs() << "mangled_name: " << mangled_name << "\n";
    Demangler.partialDemangle(mangled_name.data());
    char* fname = Demangler.getFunctionName(Buf, &len);
    outs() << "demangled_name: " << string(fname, len) << "\n";
    ExecutionEngine *EE(EngineBuilder(std::move(module)).create());
    Function *SumFn = EE->FindFunctionNamed(mangled_name);
    if (SumFn == nullptr) {
      errs() << "sumfun is null\n";
      return -1;
    }
    EE->finalizeObject();
    int (*Sum)(int, int) = (int (*)(int, int))EE->getPointerToFunction(SumFn);
    outs() << "run function\n";
    int res = Sum(4, 5);
    outs() << "Sum result: " << res << "\n";
  }

  llvm_shutdown();
  return 0;
}
