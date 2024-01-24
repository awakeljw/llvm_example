#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"

using namespace llvm;
using namespace llvm::orc;
using namespace std;

int main() {
  // 初始化LLVM
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();

  // 创建一个LLJIT实例
  auto JIT = cantFail(LLJITBuilder().create());

  // 创建一个新的LLVM模块
  orc::ThreadSafeContext ctx(std::make_unique<LLVMContext>());
  LLVMContext* Context = ctx.getContext();
  auto M = std::make_unique<Module>("test", *Context);
  M->setDataLayout(JIT->getDataLayout());

  // 创建一个简单的函数
  // Define function's signature
  IRBuilder<> builder(*Context);
  std::vector<Type *> Integers(2, builder.getInt32Ty());
  auto *funcType = FunctionType::get(builder.getInt32Ty(), Integers, false);

  // create the function "sum" and bind it to the module with ExternalLinkage,
  // so we can retrieve it later
  auto *fooFunc =
      Function::Create(funcType, Function::ExternalLinkage, "add", M.get());

  // Define the entry block and fill it with an appropriate code
  auto *entry = BasicBlock::Create(*Context, "entry", fooFunc);
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

  // 将模块添加到LLJIT
  auto TSM = ThreadSafeModule(std::move(M), ctx);
  cantFail(JIT->addIRModule(std::move(TSM)));

  // 查找并执行函数
  auto Sym = JIT->lookup("add");
  assert(Sym && "Function not found");

  int (*FP)(int, int) = (int (*)(int, int))(Sym->getAddress());
  int Result = FP(11, 12);

  outs() << "result : " << Result << "\n";
  assert(Result == 23 && "Function returned incorrect result");

  return 0;
}
