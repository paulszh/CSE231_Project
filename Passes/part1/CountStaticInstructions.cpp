#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/InstIterator.h"


using namespace llvm;

namespace {
    struct CountStaticInstructions : public FunctionPass {
        static char ID;
        std::map<String, size_t> instruction_count;
        CountStaticInstructions() : FunctionPass(ID) {}

        bool runOnFunction(Function &F) override {
            for (inst_iterator it = inst_begin(F), e = inst_end(F); it != E; ++it){
                instruction_count[it->getOpcodeName()]++; 
            }
            
           jstd::map<string,size_t>::iterator iter;
            for (iter = instruction_count.begin();iter! = instruction_count.end();++iter){
                errs() << iter->first << '\t' << iter->second << '\n';
            }
            return false;
        }
    }
}


char CountStaticInstructions::ID = 0;
static RegisterPass<CountStaticInstructions> X("cse231-csi", "CountStaticInstructions Pass",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);