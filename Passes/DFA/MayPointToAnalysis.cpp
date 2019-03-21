#include "llvm/Pass.h"
#include "231DFA.h"
#include <set>
#include <map>

using namespace llvm;

class MayPointToInfo : public Info {
    private: 
    
    public:
        // Key : identifier of IR e.g M10, R8
        std::map<std::string, std::set<std::string>> info_map;
        
        MayPointToInfo(){};
        
        MayPointToInfo(std::map<std::string, std::set<std::string>> infos){
            info_map = infos;
        };
        
        void print() {
            for (auto iter = info_map.begin(); iter != info_map.end(); iter++) {
                if(iter->second.size() == 0) {
                    continue;
                }
                
                errs() << iter->first << "->(";
                for (auto it = iter->second.begin(); it != iter->second.end(); it++) {
                    errs() << *it << "/";
                }
                errs() << ")|";
            }
            errs() << '\n';
        }
        
        void set_info_map(std::map<std::string, std::set<std::string>> s) {
            info_map = s;
        }

        static bool equals(MayPointToInfo * info1, MayPointToInfo * info2) {
            return info1->info_map == info2->info_map;
        }
        
        static MayPointToInfo * join(MayPointToInfo * info1, MayPointToInfo * info2, MayPointToInfo * result) {
            std::map<std::string, std::set<std::string>> m1 = info1->info_map;
            std::map<std::string, std::set<std::string>> m2 = info2->info_map;
            
            // TODO : insert all the elements in s2 to s1 
            for (auto iter = m2.begin(); iter != m2.end(); iter++) {
               m1[iter->first].insert(iter->second.begin(), iter->second.end());
            }
            
            result->set_info_map(m1);
            return result;
        }
};

class MayPointToAnalysis : public DataFlowAnalysis <MayPointToInfo, true> {
    private: 
        typedef std::pair<unsigned, unsigned> Edge;
        std::map<Instruction *, unsigned> instr_to_index;
        std::map<unsigned, Instruction *> index_to_instr;
        std::map<Edge, MayPointToInfo *> edge_to_info;
        std::string R = "R";
        std::string M = "M";
        
    public:
        MayPointToAnalysis(MayPointToInfo & bottom, MayPointToInfo & initialState) : DataFlowAnalysis(bottom, initialState) {};
        
        void flowfunction(Instruction * I, std::vector<unsigned> & IncomingEdges, std::vector<unsigned> & OutgoingEdges, std::vector<MayPointToInfo *> & Infos) {
            std::string opcode_name = I -> getOpcodeName();
            instr_to_index = getInstrToIndexMap();
            index_to_instr = getIndexToInstrMap();
            edge_to_info = getEdgeToInfoMap();
            
            MayPointToInfo * info_join = new MayPointToInfo();
            
            unsigned index = instr_to_index[I];

            // Join all incoming edges from three categories
            for (unsigned i = 0; i < IncomingEdges.size(); i++) {
                Edge in_edge = std::make_pair(IncomingEdges[i], index);
                MayPointToInfo::join(edge_to_info[in_edge],info_join, info_join);
            }            
            
            // A map mapping R to M
            std::map<std::string, std::set<std::string>> r2m;
            std::string string_index = std::to_string(index); 
            std::string input = R + string_index;
            std::string output = M + string_index;
            
            if (opcode_name == "alloca") {
                r2m[input].insert(output);
                MayPointToInfo::join(new MayPointToInfo(r2m), info_join, info_join);
            }
            else if (opcode_name == "bitcast" || opcode_name == "getelementptr") {
                Instruction * instr = (Instruction *) I -> getOperand(0);

                // RI -> X | RV -> X where Rv is the DFA identifier of <value>.
                if (instr_to_index.find(instr) != instr_to_index.end()) {
                    std::string ri = R + std::to_string(instr_to_index[instr]);
                    r2m[input].insert(info_join->info_map[ri].begin(), info_join->info_map[ri].end());
                    MayPointToInfo::join(new MayPointToInfo(r2m), info_join, info_join);
                }
            }
            else if (opcode_name == "load") {
                Instruction * instr = (Instruction *) I -> getOperand(0);
                
                if (instr_to_index.find(instr) != instr_to_index.end()) {
                    std::string ri = R + std::to_string(instr_to_index[instr]);
                    
                    std::set<std::string> X = info_join->info_map[ri];
                    std::set<std::string> Y;
                    for (auto it = X.begin(); it != X.end(); it++) {
                        Y.insert(info_join->info_map[*it].begin(), info_join->info_map[*it].end());
                    }
                    
                    r2m[input].insert(Y.begin(), Y.end());
                    MayPointToInfo::join(new MayPointToInfo(r2m), info_join, info_join);
                }
            }
            else if (opcode_name == "store") {
                // Rv and Rp are the DFA identifiers of <value> and <pointer>, respectively.
                Instruction * value = (Instruction *) I -> getOperand(0);
                Instruction * pointer = (Instruction *) I -> getOperand(1);
                
                if (instr_to_index.find(value) != instr_to_index.end() && instr_to_index.find(pointer) != instr_to_index.end()) {
                    std::string rv = R + std::to_string(instr_to_index[value]);
                    std::string rp = R + std::to_string(instr_to_index[pointer]);
                    
                    std::set<std::string> X = info_join->info_map[rv];
                    std::set<std::string> Y = info_join->info_map[rp];
                    
                    for (auto it = Y.begin(); it != Y.end(); it++) {
                        r2m[*it].insert(X.begin(), X.end());
                    }
                    
                    MayPointToInfo::join(new MayPointToInfo(r2m), info_join, info_join);
                }
            }
            else if (opcode_name == "select") {
                // R1 and R2 are the DFA identifiers of <val1> and <val2>, respectively.
                Instruction * val1 = (Instruction *) I -> getOperand(1);
                Instruction * val2 = (Instruction *) I -> getOperand(2);
                
                if (instr_to_index.find(val1) != instr_to_index.end()) {
                    std::string r1 = R + std::to_string(instr_to_index[val1]);
                    r2m[input].insert(info_join->info_map[r1].begin(), info_join->info_map[r1].end());
                }
                
                if (instr_to_index.find(val2) != instr_to_index.end()) {
                    std::string r2 = R + std::to_string(instr_to_index[val2]);
                    r2m[input].insert(info_join->info_map[r2].begin(), info_join->info_map[r2].end());
                }

                MayPointToInfo::join(new MayPointToInfo(r2m), info_join, info_join);
            }
            else if (opcode_name == "phi") {
                //  R0 through Rk are the DFA identifiers of <val0> through <valk>
                Instruction * first_non_phi_instr = I->getParent()->getFirstNonPHI();
                
                unsigned n = getInstrToIndexMap()[first_non_phi_instr];

                for (unsigned i = index; i < n; i ++) {
                    Instruction * phi  = index_to_instr[i];
                    for (unsigned j = 0; j < phi->getNumOperands(); j++) {
                        Instruction * val = (Instruction *) phi->getOperand(j);
                        if (instr_to_index.find(val) != instr_to_index.end()) {
                            std::string ri = R + std::to_string(instr_to_index[val]);
                            r2m[input].insert(info_join->info_map[ri].begin(), info_join->info_map[ri].end());
                        }
                    }
                }

                MayPointToInfo::join(new MayPointToInfo(r2m), info_join, info_join);
            }
            else { // All other instructions
                // Do nothing because OUT = IN
            }

            for (unsigned i = 0; i < OutgoingEdges.size(); i++) {
                    Infos.push_back(info_join);
            }
        }
};

namespace {
    struct MayPointToAnalysisPass : public FunctionPass {
        static char ID;
        MayPointToAnalysisPass() : FunctionPass(ID) {}
        
        bool runOnFunction(Function &F) override {
            MayPointToInfo bottom; 
            MayPointToInfo initialState;
            MayPointToAnalysis new_analysis(bottom, initialState);
            
            new_analysis.runWorklistAlgorithm(&F);
            new_analysis.print();
            return false;
        }
    };
}

char MayPointToAnalysisPass::ID = 0;
static RegisterPass<MayPointToAnalysisPass> X("cse231-maypointto", "MayPointToAnalysis",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);