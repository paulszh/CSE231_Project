#include "llvm/Pass.h"
#include "231DFA.h"
#include <set>
#include <unordered_set>

using namespace llvm;

class ReachingInfo : public Info {
    private: 
        std::set<unsigned> info_set;

    public:
        ReachingInfo(){};
        
        ReachingInfo(std::set<unsigned> infos){
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
        
        static bool equals(ReachingInfo * info1, ReachingInfo* info2) {
            return info1->info_set == info2->info_set;
        }
        
        static ReachingInfo* join(ReachingInfo * info1, ReachingInfo * info2, ReachingInfo * result) {
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

class ReachingDefinitionAnalysis : public DataFlowAnalysis <ReachingInfo, true> {
    private:
        typedef std::pair<unsigned, unsigned> Edge;
        std::map<Instruction *, unsigned> instr_to_index;
        std::map<Edge, ReachingInfo *> edge_to_info;
        std::unordered_set<std::string> category_one = {"alloca", "load", "select", "icmp", "fcmp", "getelementptr"};

    public:
        
        ReachingDefinitionAnalysis(ReachingInfo & bottom, ReachingInfo & initialState) : DataFlowAnalysis(bottom, initialState) {}

        void flowfunction(Instruction * I, std::vector<unsigned> & IncomingEdges, std::vector<unsigned> & OutgoingEdges, std::vector<ReachingInfo *> & Infos) {
            
            int category;
            std::string opcode_name = I -> getOpcodeName();
            instr_to_index = getInstrToIndexMap();
            edge_to_info = getEdgeToInfoMap();
            
            
            unsigned index = instr_to_index[I];
            ReachingInfo * info_union = new ReachingInfo();
            
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

            // Join all incoming edges for all three categories: 
            for (unsigned i = 0; i < IncomingEdges.size(); i++) {
                Edge in_edge = std::make_pair(IncomingEdges[i], index);
                ReachingInfo::join(info_union, edge_to_info[in_edge], info_union);
            }       
            
            if (category == 1) {
                std::set<unsigned> tmp;
                tmp.insert(index);
                ReachingInfo::join(new ReachingInfo(tmp), info_union, info_union);
            }
            else if (category == 2) {
                // do nothing
            }
            else {
                // Thrid category
                Instruction * first_non_phi_instr = I->getParent()->getFirstNonPHI();
                std::set<unsigned> tmp;
                
                unsigned n = getInstrToIndexMap()[first_non_phi_instr];
                for (unsigned i = index; i < n; i++) {
                    // add all the phi instructions
                    tmp.insert(i);
                }
                ReachingInfo::join(new ReachingInfo(tmp), info_union, info_union);
            }
            
            for (unsigned i = 0; i < OutgoingEdges.size(); i++) {
                Infos.push_back(info_union);
            }
        }   
};


namespace {
struct ReachingDefinitionAnalysisPass : public FunctionPass {
    static char ID;
    ReachingDefinitionAnalysisPass() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {
        ReachingInfo bottom;
        ReachingInfo initialState;
        ReachingDefinitionAnalysis new_analysis(bottom, initialState);

        new_analysis.runWorklistAlgorithm(&F);
        new_analysis.print();
        return false;
    }
};
}

char ReachingDefinitionAnalysisPass::ID = 0;
static RegisterPass<ReachingDefinitionAnalysisPass> X("cse231-reaching", "ReachingDefinitionAnalysis",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);