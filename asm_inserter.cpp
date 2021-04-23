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
    // AllocaInst* alloc;
    LoadInst* load;
  };

  bool runOnFunction(Function &F) override {
    FILE* log_file = fopen ("logs.log", "w");
    std::map<std::string, DataAboutFunction> funcs_map;

    bool changed = false;
    for (BasicBlock& BB : F) {
      // llvm::Type *type = Type::getInt32Ty(F.getContext());
      // llvm::AllocaInst (type, 0, "", &(BB.front()));

      for (Instruction& I : BB) {
        if (I.getParent() != &BB)
          fprintf (log_file, "Parent not equal!\n");
        if (I.getOpcode() != Instruction::Call) {
          continue;
        }
        // I.dump();
        // llvm::Type *type = Type::getInt32Ty(F.getContext());
        // AllocaInst *alloc = new llvm::AllocaInst (type, 0, "", &I);

        auto CI = cast<CallInst>(&I);
        
        if (CI->getCalledFunction()->isIntrinsic()) {
          continue;
        }
        Function* func = CI->getCalledFunction();
        std::string name_func = std::string (func->getName());

        auto fp_type = PointerType::get(CI->getFunctionType(), 0);

        auto ret_search = funcs_map.find (name_func);

        if (ret_search != funcs_map.end()) {
          fprintf (log_file, "Function \"%-20s\" - \"%-20s\"\tFOUND\n", std::string (F.getName()).c_str(), name_func.c_str());
          // StoreInst* store = new StoreInst(CI->getCalledFunction(), ret_search->second.alloc, CI);
          // LoadInst* load = new LoadInst(fp_type, ret_search->second.alloc, "", CI);
          CI->setCalledFunction(CI->getFunctionType(), ret_search->second.load);
        }
        else {
          Instruction* insert_instr = &*(F.getEntryBlock().getFirstInsertionPt());
          AllocaInst* alloc = new AllocaInst(fp_type, 0, "", insert_instr);
          StoreInst* store = new StoreInst(CI->getCalledFunction(), alloc, insert_instr);
          LoadInst* load = new LoadInst(fp_type, alloc, "", insert_instr);
          // CI->setCalledFunction(CI->getFunctionType(), load);


          fprintf (log_file, "Function \"%-20s\" - \"%-20s\"\tNOT FOUND\n", std::string (F.getName()).c_str(), name_func.c_str());
          auto iasm_type = FunctionType::get(fp_type, {fp_type}, false);
          auto iasm = InlineAsm::get(iasm_type, "", "=r,0", false);
          auto iasm_call_t = cast<FunctionType>(iasm->getType()->getPointerElementType());


          auto call = CallInst::Create(iasm_call_t, iasm, load, "", CI);
          CI->setCalledFunction(CI->getFunctionType(), call);
          // CI->setCalledFunction(CI->getFunctionType(), load);

          funcs_map[name_func] = {func, /*alloc*/load};

          // delete load;
          // delete store;

          // Allocate mem for point
          // IntegerType::get(llvm::getGlobalContext(), 32); 
          // Type *void_type = Type::getVoidTy(F.getContext())->getPointerTo(0);

          // Type *void_type = Type::getPrimitiveType (F.getContext(), Type::TokenTyID);
          // Type *void_type = llvm::Type::getInt8PtrTy (F.getContext());

          

          // std::vector<value*> vec;
          // ArrayRef<Value*>* array_ref = new ArrayRef<Value*>(func->getArg(0));

          // auto new_call = CallInst::Create(CI->getFunctionType(), load, func->getFunctionType()->params(), "", CI);
          // new_call->setCalledFunction(func);

          // CI->getModule()->getOrInsertFunction();


        }
      }
    }
    fclose (log_file);
    return changed;
  }

  // bool runOnFunction(Function &F) override {
  //   bool changed = false;
  //   for (BasicBlock& BB : F) {
  //     for (Instruction& I : BB) {
  //       llvm::Type *type = Type::getInt32Ty(F.getContext());
  //       AllocaInst* smth = new llvm::AllocaInst (type, 0, "", &I);
  //     }
  //   }
  //   return changed;
  // }
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
