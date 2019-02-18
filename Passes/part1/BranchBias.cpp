#include "llvm/IR/Type.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
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
            BranchInst *branch = dyn_cast<BranchInst>(B -> getTerminator());
            
            if (branch != NULL) {
		        if (branch->isConditional()) {
                    vector<Value *> args;
                    args.push_back(branch->getCondition());
                    
                    Builder.CreateCall(update_func, args);
                }
		    }
            
            string opcode = (string)(B->getTerminator())->getOpcodeName();
            if (opcode == "ret") {
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

