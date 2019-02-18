#include "llvm/Pass.h"
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
struct CountDynamicInstructions : public FunctionPass {
    static char ID;
    unordered_map<string, size_t> instruction_count;
    CountDynamicInstructions() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {
        Module *mod = F.getParent();
        LLVMContext &context = mod->getContext();

        Constant *update_func = mod->getOrInsertFunction(
                                "updateInstrInfo",               // name of function
                                Type::getVoidTy(context),        // return type
                                Type::getInt32Ty(context),       
                                Type::getInt32PtrTy(context),
                                Type::getInt32PtrTy(context));      
        

        Constant *print_func = mod->getOrInsertFunction(
                               "printOutInstrInfo",             // name of function
                                Type::getVoidTy(context));       // return type
        
        
        // Iterating through fucntion here
        // Here B is a pointer to a basic block
        for (Function::iterator B = F.begin(), BE = F.end(); B != BE; ++B) {
            unordered_map<int, int> op_count;
            for (BasicBlock::iterator I = B->begin(), IE = B->end(); I != IE; ++I) {
                op_count[I->getOpcode()]++;
            }

            IRBuilder<> Builder(&*B);
            Builder.SetInsertPoint(B->getTerminator());
            
            int count = op_count.size();
            vector <Value*> args;
            vector <Constant*> keys;
            vector <Constant*> values;
            
            // Repeat the static instruction count step here
            for (unordered_map<int, int>::iterator it = op_count.begin(); it != op_count.end(); it++) {
                Constant * key = ConstantInt::get(Type::getInt32Ty(context), it->first);
                Constant * value = ConstantInt::get(Type::getInt32Ty(context), it->second);
                keys.push_back(key);
                values.push_back(value);
            }
            
            ArrayType* array_ty = ArrayType::get(Type::getInt32Ty(context), count);

            GlobalVariable * key_table = new GlobalVariable(*mod, 
                                        array_ty, 
                                        true, 
                                        GlobalValue::InternalLinkage, 
                                        ConstantArray::get(array_ty,keys), 
                                        "global_keys"); 
            GlobalVariable * value_table = new GlobalVariable(*mod, 
                                        array_ty, 
                                        true, 
                                        GlobalValue::InternalLinkage, 
                                        ConstantArray::get(array_ty,values), 
                                        "global_values"); 
            
            Value* indices[2] = {ConstantInt::get(Type::getInt32Ty(context),0), ConstantInt::get(Type::getInt32Ty(context),0)};
            Value * zero = Builder.CreateInBoundsGEP(key_table, indices);
            Value * one = Builder.CreateInBoundsGEP(value_table, indices);
            // initialize the args before calling the update function;
            args.push_back(ConstantInt::get(Type::getInt32Ty(context), count));
            args.push_back(zero);
            args.push_back(one);
            
            Builder.CreateCall(update_func, args);

            string opcode = (string)(B->getTerminator())->getOpcodeName();
            if (opcode == "ret") {
                // print the count of each instruction before return
                Builder.CreateCall(print_func);   
            }
        }
        return false;
    }
};
}



char CountDynamicInstructions::ID = 0;
static RegisterPass<CountDynamicInstructions> X("cse231-cdi", "CountDynamicInstruction",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);