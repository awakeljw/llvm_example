
#include <iostream>

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Mangler.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
// Optimizations
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;
using namespace std;
using namespace llvm::object;

class SimpleResolver : public LegacyJITSymbolResolver {
public:
  ~SimpleResolver() override = default;
  JITSymbol findSymbol(const std::string &Name) override {
    return JITSymbol(nullptr);
  }

  JITSymbol findSymbolInLogicalDylib(const std::string &Name) override {
    return JITSymbol(nullptr);
  }
};

static std::string getMangledName(const std::string &name_to_mangle,
                                  llvm::DataLayout &layout) {
  std::string mangled_name;
  llvm::raw_string_ostream mangled_name_stream(mangled_name);
  llvm::Mangler::getNameWithPrefix(mangled_name_stream, name_to_mangle, layout);
  mangled_name_stream.flush();

  return mangled_name;
}

llvm::Function *createAddFunction(Module *module) {
  /* Builds the following function:

  int sum(int a, int b) {
      int result = a + b;
      return result;
  }
  */

  LLVMContext &context = module->getContext();
  IRBuilder<> builder(context);

  // Define function's signature
  std::vector<Type *> Integers(2, builder.getInt32Ty());
  auto *funcType = FunctionType::get(builder.getInt32Ty(), Integers, false);

  // create the function "sum" and bind it to the module with ExternalLinkage,
  // so we can retrieve it later
  auto *fooFunc =
      Function::Create(funcType, Function::ExternalLinkage, "add", module);

  // Define the entry block and fill it with an appropriate code
  auto *entry = BasicBlock::Create(context, "entry", fooFunc);
  builder.SetInsertPoint(entry);

  // Retrieve arguments and proceed with further adding...
  auto args = fooFunc->arg_begin();
  Value *arg1 = &(*args);
  args = std::next(args);
  Value *arg2 = &(*args);
  auto *result = builder.CreateAdd(arg1, arg2, "result");

  // ...and return
  builder.CreateRet(result);

  // Verify at the end
  verifyFunction(*fooFunc);
  return fooFunc;
};

llvm::Function *createSumFunction(Module *module) {
  /* Builds the following function:

  int sum(int a, int b) {
      int sum1 = 1 + 1;
      int sum2 = sum1 + a;
      int result = sum2 + b;
      return result;
  }
  */

  LLVMContext &context = module->getContext();
  IRBuilder<> builder(context);

  // Define function's signature
  std::vector<Type *> Integers(2, builder.getInt32Ty());
  auto *funcType = FunctionType::get(builder.getInt32Ty(), Integers, false);

  // create the function "sum" and bind it to the module with ExternalLinkage,
  // so we can retrieve it later
  auto *fooFunc =
      Function::Create(funcType, Function::ExternalLinkage, "sum", module);

  // Define the entry block and fill it with an appropriate code
  auto *entry = BasicBlock::Create(context, "entry", fooFunc);
  builder.SetInsertPoint(entry);

  // Add constant to itself, to visualize constant folding
  Value *constant = ConstantInt::get(builder.getInt32Ty(), 0x1);
  auto *sum1 = builder.CreateAdd(constant, constant, "sum1");

  // Retrieve arguments and proceed with further adding...
  auto args = fooFunc->arg_begin();
  Value *arg1 = &(*args);
  args = std::next(args);
  Value *arg2 = &(*args);
  auto *sum2 = builder.CreateAdd(sum1, arg1, "sum2");
  auto *result = builder.CreateAdd(sum2, arg2, "result");

  // ...and return
  builder.CreateRet(result);

  // Verify at the end
  verifyFunction(*fooFunc);
  return fooFunc;
};

static void runOptimizationPassOnModule(Module &module) {
  llvm::PassManagerBuilder builder;
  llvm::legacy::PassManager mpm;
  auto fpm = std::make_unique<legacy::FunctionPassManager>(&module);
  builder.OptLevel = 3;
  builder.SizeLevel = 1;
  builder.LoopVectorize = true;
  builder.SLPVectorize = true;
  builder.populateFunctionPassManager(*fpm);
  builder.populateModulePassManager(mpm);
  // For each function in the module
  for (auto & func : module) {
    // Run the FPM on this function
    fpm->run(func);
  }
  fpm->doFinalization();
};

int main(int argc, char *argv[]) {
  // Initilaze native target
  llvm::TargetOptions Opts;
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  Triple triple(sys::getDefaultTargetTriple());
  std::string error;
  const Target *target =
      TargetRegistry::lookupTarget(triple.getTriple(), error);
  TargetOptions options;
  auto RM = Optional<Reloc::Model>();
  std::unique_ptr<TargetMachine> targetMachine(target->createTargetMachine(
      triple.getTriple(), "generic", "", options, RM));

  llvm::DataLayout layout{targetMachine->createDataLayout()};
  LLVMContext context;
  auto myModule = make_unique<Module>("My First JIT", context);
  auto *module = myModule.get();
  module->setDataLayout(layout);
  module->setTargetTriple(triple.getTriple());
  auto *func = createSumFunction(module); // create function
  auto *fun2 = createAddFunction(module);
  //  std::vector<Type*> TwoInt32(2, Type::getInt32Ty(context));
  //  FunctionType *FT = FunctionType::get(Type::getInt32Ty(context), TwoInt32,
  //  false);
  //
  //  // Add the function to the module
  //  FunctionCallee FC = module->getOrInsertFunction("add", FT);

  runOptimizationPassOnModule(*module);

  SmallVector<char, 0> buffer;
  raw_svector_ostream stream(buffer);
  legacy::PassManager passManager;

  // 将 Module 转换为目标代码
  if (targetMachine->addPassesToEmitFile(passManager, stream, nullptr,
                                         CGFT_ObjectFile)) {
    return -1;
  }

  passManager.run(*module);

  std::unique_ptr<MemoryBuffer> memBuffer =
      MemoryBuffer::getMemBufferCopy(stream.str());
  Expected<std::unique_ptr<ObjectFile>> objFileOrErr =
      ObjectFile::createObjectFile(memBuffer->getMemBufferRef());

  if (!objFileOrErr) {
    return -1;
  }

  std::unique_ptr<object::ObjectFile> objectFile =
      std::move(objFileOrErr.get());

  auto resolver{std::make_unique<SimpleResolver>()};
  // 创建一个 SectionMemoryManager
  std::unique_ptr<SectionMemoryManager> memoryManager =
      std::make_unique<SectionMemoryManager>();

  // 创建一个 RuntimeDyld
  RuntimeDyld runtimeDyld(*memoryManager, *resolver);

  // 加载 ObjectFile
  runtimeDyld.loadObject(*objectFile);

  // 解析和链接符号
  runtimeDyld.finalizeWithMemoryManagerLocking();

  for (auto &func : *module) {
    auto func_name = std::string(func.getName());
    std::cout << "function name:" << func_name << std::endl;
    auto mangled_name = getMangledName(func_name, layout);
    auto jit_symbol = runtimeDyld.getSymbol(mangled_name);

    if (!jit_symbol) {
      std::cout << "do not found jit symbol" << std::endl;
      return -1;
    }

    auto *jit_symbol_address =
        reinterpret_cast<void *>(jit_symbol.getAddress());

    auto *func_ptr = (int (*)(int, int))jit_symbol_address;
    std::cout << "func ptr addr:" << jit_symbol_address << std::endl;
    if (mangled_name == "sum") {
      // Execute
      int arg1 = 5;
      int arg2 = 7;
      int result = func_ptr(arg1, arg2);
      std::cout << arg1 << " + " << arg2 << " + 1 + 1 = " << result
                << std::endl;
    } else if (mangled_name == "add") {
      // Execute
      int arg1 = 5;
      int arg2 = 7;
      int result = func_ptr(arg1, arg2);
      std::cout << arg1 << " + " << arg2 << " = " << result << std::endl;
    }
  }
  return 0;
}
