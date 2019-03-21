add_llvm_library( submission_pt3 MODULE
  ReachingDefinitionAnalysis.cpp 
  LivenessAnalysis.cpp
  MayPointToAnalysis.cpp
  231DFA.h

  PLUGIN_TOOL
  opt
  )
