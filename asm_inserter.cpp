#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"


#include <iostream>
#include <cstdio>
#include <map>


using namespace llvm;

namespace {
struct AsmInserter : public FunctionPass {
  static char ID;
  AsmInserter() : FunctionPass(ID) {}

  struct DataAboutFunction{
    Function* func;
    Value*    call;
  };

  bool runOnFunction(Function &F) override {
    std::map<std::string, DataAboutFunction> funcs_map;

    bool changed = false;
    for (BasicBlock& BB : F) {
      for (Instruction& I : BB) {
        if (I.getOpcode() != Instruction::Call) {
          continue;
        }
        auto CI = cast<CallInst>(&I);
        Function* func = CI->getCalledFunction();

        if (func->isIntrinsic()) {
          continue;
        }
        std::string name_func (func->getName());

        auto ret_search = funcs_map.find (name_func);
        if (ret_search != funcs_map.end()) {
          CI->setCalledFunction(CI->getFunctionType(), ret_search->second.call);
        }
        else {
          Instruction* insert_instr = &*(F.getEntryBlock().getFirstInsertionPt());

          auto fp_type = PointerType::get(CI->getFunctionType(), 0);
          auto iasm_type = FunctionType::get(fp_type, {fp_type}, false);
          auto iasm = InlineAsm::get(iasm_type, "", "=r,0", false);
          auto iasm_call_t = cast<FunctionType>(iasm->getType()->getPointerElementType());
          auto call = CallInst::Create(iasm_call_t, iasm, func, "", insert_instr);
          CI->setCalledFunction(CI->getFunctionType(), call);

          funcs_map[name_func] = {func, call};
        }
      }
    }
    return changed;
  }
}; // end of struct AsmInserter
}  // end of anonymous namespace

char AsmInserter::ID = 0;
static RegisterPass<AsmInserter> X("asm_inserter", "AsmInserter Pass",
                                   false /* Only looks at CFG */,
                                   false /* Analysis Pass */);

static RegisterStandardPasses Y(
    PassManagerBuilder::EP_EarlyAsPossible,
    [](const PassManagerBuilder &Builder,
       legacy::PassManagerBase &PM) { PM.add(new AsmInserter()); });
