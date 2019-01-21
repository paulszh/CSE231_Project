#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/InstIterator.h"
#include <unordered_map>


using namespace llvm;
using namespace std;

namespace {
    struct CountStaticInstructions : public FunctionPass {
        static char ID;
        unordered_map<string, size_t> instruction_count;
        CountStaticInstructions() : FunctionPass(ID) {}

        bool runOnFunction(Function &F) override {
            for (inst_iterator it = inst_begin(F), E = inst_end(F); it != E; ++it) {
                instruction_count[it->getOpcodeName()]++; 
            }
            
            unordered_map<string,size_t>::iterator iter;
            for (iter = instruction_count.begin(); iter != instruction_count.end(); iter++) {
                errs() << iter->first << '\t' << iter->second << '\n';
            }
            return false;
        }
    };
}

char CountStaticInstructions::ID = 0;
static RegisterPass<CountStaticInstructions> X("cse231-csi", "CountStaticInstructions",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);