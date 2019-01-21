#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include<unordered_map>
#include<vector>

using namespace llvm;
using namespace std;

namespace {
    struct BranchBias : public FunctionPass {
    static char ID;
    unordered_map<string, size_t> instruction_count;
    BranchBias() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {
        Module *mod = F.getParent();
        LLVMContext &context = mod->getContext();

        Constant *update_func = mod->getOrInsertFunction(
                                "updateBranchInfo",               // name of function
                                Type::getVoidTy(context),        // return type
                                Type::getInt1Ty(context));      
        

        Constant *print_func = mod->getOrInsertFunction(
                               "printOutBranchInfo",             // name of function
                                Type::getVoidTy(context));       // return type
        
        
        // Iterating through fucntion here
        for (Function::iterator B = F.begin(), BE = F.end(); B != BE; ++B) {

            IRBuilder<> Builder(&*B);
            Builder.SetInsertPoint(B->getTerminator());

            BranchInst *branch_inst = dyn_cast<BranchInst>(B -> getTerminator());
            if (branch_inst != NULL && branch_inst->isConditional()) {
                vector<Value *> args;
                args.push_back(branch_inst->getCondition());
                Builder.CreateCall(update_func, args);
            }
            
            if ((string)(B->getTerminator())->getOpcodeName() == "ret") {
              Builder.CreateCall(print_func);   
            }
        }
        return false;
    }
};
}

char BranchBias::ID = 0;
static RegisterPass<BranchBias> X("cse231-bb", "BranchBias",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);

