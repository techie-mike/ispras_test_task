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

  bool runOnFunction(Function &F) override {
    FILE* log_file = fopen ("logs.log", "w");
    std::map<std::string, Function*> funcs_map;

    bool changed = false;
    for (BasicBlock& BB : F) {
      for (Instruction& I : BB) {
        if (I.getOpcode() != Instruction::Call) {
          continue;
        }
        auto CI = cast<CallInst>(&I);
        if (CI->getCalledFunction()->isIntrinsic()) {
          continue;
        }
        Function* func = CI->getCalledFunction();
        std::string name_func = std::string (func->getName());

        auto ret_search = funcs_map.find (name_func);
        if (ret_search != funcs_map.end()) {
          fprintf (log_file, "Function \"%-20s\" - \"%-20s\"\tFOUND\n", std::string (F.getName()).c_str(), name_func.c_str());
        }
        else {
          fprintf (log_file, "Function \"%-20s\" - \"%-20s\"\tNOT FOUND\n", std::string (F.getName()).c_str(), name_func.c_str());
          auto fp_type = PointerType::get(CI->getFunctionType(), 0);
          auto iasm_type = FunctionType::get(fp_type, {fp_type}, false);
          auto iasm = InlineAsm::get(iasm_type, "", "=r,0", false);
          auto iasm_call_t = cast<FunctionType>(iasm->getType()->getPointerElementType());
          auto call = CallInst::Create(iasm_call_t, iasm, CI->getCalledFunction(), "", CI);
          CI->setCalledFunction(CI->getFunctionType(), call);

          funcs_map[name_func] = func;

          // CI->getModule()->getOrInsertFunction();
        }
      }
    }
    fclose (log_file);
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
