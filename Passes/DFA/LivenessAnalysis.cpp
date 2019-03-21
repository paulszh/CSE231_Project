#include "llvm/Pass.h"
#include "231DFA.h"
#include <set>
#include <unordered_set>

using namespace llvm;

class LivenessInfo : public Info {
    private: 
    
    public:
        std::set<unsigned> info_set;
        
        LivenessInfo(){};
        
        LivenessInfo(std::set<unsigned> infos){
            info_set = infos;
        };
        
        void print() {
            for (auto iter = info_set.begin(); iter != info_set.end(); iter++) {
                errs() << *iter << '|';
            }
            errs() << '\n';
        }
        
        void set_info_set(std::set <unsigned> s) {
            info_set = s;
        }

        static bool equals(LivenessInfo * info1, LivenessInfo * info2) {
            return info1->info_set == info2->info_set;
        }
        
        static LivenessInfo * join(LivenessInfo * info1, LivenessInfo * info2, LivenessInfo * result) {
            std::set<unsigned> s1 = info1->info_set;
            std::set<unsigned> s2 = info2->info_set;
            
            // insert all the elements in s2 to s1
            for (auto iter = s2.begin(); iter != s2.end(); iter++) {
                s1.insert(*iter);
            }
            
            result->set_info_set(s1);
            return result;
        }
};

class LivenessAnalysis : public DataFlowAnalysis<LivenessInfo, false> {
    private:
        typedef std::pair<unsigned, unsigned> Edge;
        std::map<Instruction *, unsigned> instr_to_index;
        std::map<unsigned, Instruction *> index_to_instr;
        std::map<Edge, LivenessInfo *> edge_to_info;
        std::unordered_set<std::string> category_one = {"alloca", "load", "select", "icmp", "fcmp", "getelementptr"};

    public: 
        LivenessAnalysis(LivenessInfo & bottom, LivenessInfo & initialState) : DataFlowAnalysis(bottom, initialState) {}
        
        void flowfunction(Instruction * I, std::vector<unsigned> & IncomingEdges, std::vector<unsigned> & OutgoingEdges, std::vector<LivenessInfo *> & Infos) {
            int category; 
            std::string opcode_name = I -> getOpcodeName();
            instr_to_index = getInstrToIndexMap();
            index_to_instr = getIndexToInstrMap();
            edge_to_info = getEdgeToInfoMap();
            
            unsigned index = instr_to_index[I];
            LivenessInfo * info_join = new LivenessInfo();
            
            if (I -> isBinaryOp() || category_one.find(opcode_name) != category_one.end()) {
                // First Category : IR Instructions that return a value
                category = 1;
            }
            else if (opcode_name == "phi") { 
                // Third Category : Phi instructions
                category = 3;
            }
            else {
                // Second Category: IR instructions that do not return a value
                category = 2;
            }
            
            //operands
            std::set<unsigned> ops;
            for (unsigned i = 0; i < I->getNumOperands(); i++) {
                Instruction * op = (Instruction *) I -> getOperand(i);
                
                if (instr_to_index.find(op) != instr_to_index.end()) {
                    ops.insert(instr_to_index[op]);
                }
            }
            
            // Join all incoming edges from three categories
            for (unsigned i = 0; i < IncomingEdges.size(); i++) {
                Edge in_edge = std::make_pair(IncomingEdges[i], index);
                LivenessInfo::join(info_join, edge_to_info[in_edge], info_join);
            }
            
            if (category == 1) {
                // join all inputs and operands and subtract index
                LivenessInfo::join(info_join, new LivenessInfo(ops), info_join);
                info_join->info_set.erase(index);
                
                for (unsigned i = 0; i < OutgoingEdges.size(); i++) {
                    Infos.push_back(info_join);
                }
            }
            else if (category == 2) {
                // join all inputs and operands without subtracting index
                LivenessInfo::join(info_join, new LivenessInfo(ops), info_join);
                
                for (unsigned i = 0; i < OutgoingEdges.size(); i++) {
                    Infos.push_back(info_join);
                }
            }
            else {
                Instruction * first_non_phi_instr = I->getParent()->getFirstNonPHI();
                
                unsigned n = getInstrToIndexMap()[first_non_phi_instr];
                for (unsigned i = index; i < n; i++) {
                    info_join->info_set.erase(i);
                }
                
                // Handle each output respectively
                for (unsigned i = 0;  i < OutgoingEdges.size(); i++) {
                    std::set<unsigned> tmp;
                    
                    for (unsigned j = index; j < n; j ++) {
                        Instruction * curr = index_to_instr[j];
                        
                        for (unsigned k = 0; k < curr->getNumOperands(); k++) {
                            Instruction * op = (Instruction *)curr->getOperand(k);
                            
                            if (instr_to_index.find(op) != instr_to_index.end() && 
                                op->getParent() == index_to_instr[OutgoingEdges[i]]->getParent()) {
                                tmp.insert(instr_to_index[op]);
                                break;
                            }
                        }
                    }

                    LivenessInfo * result = new LivenessInfo();
                    LivenessInfo::join(info_join, new LivenessInfo(tmp), result);
                    Infos.push_back(result);
                }
            }
        }
};

namespace {
    struct LivenessAnalysisPass : public FunctionPass {
        static char ID;
        LivenessAnalysisPass() : FunctionPass(ID) {}
        
        bool runOnFunction(Function &F) override {
            LivenessInfo bottom; 
            LivenessInfo initialState;
            LivenessAnalysis new_analysis(bottom, initialState);
            
            new_analysis.runWorklistAlgorithm(&F);
            new_analysis.print();
            return false;
        }
    };
}

char LivenessAnalysisPass::ID = 0;
static RegisterPass<LivenessAnalysisPass> X("cse231-liveness", "LivenessAnalysis",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);


  