#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;

namespace {
struct RegInserter : public FunctionPass {
  static char ID;
  RegInserter() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override {
    bool changed = false;
    auto *M = F.getParent();
    auto &C = F.getContext();
    auto int64_ty = Type::getInt64Ty(C);
    auto void_ptr = PointerType::getInt8PtrTy(C, 0);
    auto MD = MetadataAsValue::get(C, MDNode::get(C, {MDString::get(C, "x28")}));

    // Initialize x28 reg
    if (F.getName() == "main") {
      auto &EBB = F.getEntryBlock();
      auto &I = *EBB.getFirstInsertionPt();
      // void *y;
      auto alloca = new AllocaInst(void_ptr, 0, "", &I);
      // x28 = &y;
      auto ptr_cast = new PtrToIntInst(alloca, int64_ty , "", &I);
      Function *WriteRegister = Intrinsic::getDeclaration(M, Intrinsic::write_register, int64_ty);
      CallInst::Create(WriteRegister->getFunctionType(), WriteRegister, {MD, ptr_cast}, "", &I);
      // y = x;
      Function *ReadRegister = Intrinsic::getDeclaration(M, Intrinsic::read_register, int64_ty);
      auto call = CallInst::Create(ReadRegister->getFunctionType(), ReadRegister, {MD}, "", &I);
      auto int_cast = new IntToPtrInst(call, void_ptr, "", &I);
      new StoreInst(int_cast, alloca, &I);

      changed = true;
    }
    // Instrument all calls with load from x28: x = *(void **) x;
    for (BasicBlock& BB : F) {
      for (Instruction& I : BB) {
        if(I.getOpcode() != Instruction::Call) {
          continue;
        }
        auto CI = cast<CallInst>(&I);
        if (CI->getCalledFunction()->isIntrinsic()) {
          continue;
        }
        Function *ReadRegister = Intrinsic::getDeclaration(M, Intrinsic::read_register, int64_ty);
        auto call = CallInst::Create(ReadRegister->getFunctionType(), ReadRegister, {MD}, "", &I);
        auto int_cast = new IntToPtrInst(call, PointerType::get(void_ptr, 0), "", &I);

        auto load = new LoadInst(void_ptr, int_cast, "", &I);

        auto ptr_cast = new PtrToIntInst(load, int64_ty , "", &I);
        Function *WriteRegister = Intrinsic::getDeclaration(M, Intrinsic::write_register, int64_ty);
        CallInst::Create(WriteRegister->getFunctionType(), WriteRegister, {MD, ptr_cast}, "", &I);

        changed = true;
      }
    }
    return changed;
  }
}; // end of struct RegInserter
}  // end of anonymous namespace

char RegInserter::ID = 0;
static RegisterPass<RegInserter> X("reg_inserter", "RegInserter Pass",
                                   false /* Only looks at CFG */,
                                   false /* Analysis Pass */);

static RegisterStandardPasses Y(
    PassManagerBuilder::EP_EarlyAsPossible,
    [](const PassManagerBuilder &Builder,
       legacy::PassManagerBase &PM) { PM.add(new RegInserter()); });
