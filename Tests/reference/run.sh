#!/bin/bash

# path to clang++, llvm-dis, and opt
#/LVM_BIN=/LLVM_ROOT/build/bin
# path to CSE231.so
LLVM_SO=/LLVM_ROOT/build/lib
# path to lib231.c
LIB_DIR=/lib231
# path to the test directory
TEST_DIR=/tests/mounted_test/reference/

FLAGS=`llvm-config --system-libs --cppflags --ldflags --libs core`
FLAGS="$FLAGS -Wno-unused-command-line-argument"

# Testing for part1
clang -c -O0 $TEST_DIR/test1.c -emit-llvm -S -o /tmp/test1-c.ll
clang++ -c -O0 $TEST_DIR/test1.cpp -emit-llvm -S -o /tmp/test1.ll
clang++ -c $TEST_DIR/test1-main.cpp -emit-llvm -S -o /tmp/test1-main.ll
clang++ -c $TEST_DIR/myTest.cpp -emit-llvm -S -o /tmp/myTest.ll

#clang++ -c $LIB_DIR/lib231.cpp -emit-llvm -S -o /tmp/lib231.ll

clang++ /lib231/lib231.cpp -emit-llvm -S $FLAGS -o /tmp/lib231.bc 

# Solution for part1 

opt -load $LLVM_SO/231_solution.so -cse231-csi < /tmp/test1-c.ll > /dev/null 2> /output/csi.solution
opt -load $LLVM_SO/231_solution.so -cse231-csi < /tmp/test1.ll > /dev/null 2>> /output/csi.solution
opt -load $LLVM_SO/231_solution.so -cse231-csi < /tmp/test1-c.ll > /dev/null 2>> /output/si.solution
opt -load $LLVM_SO/231_solution.so -cse231-csi < /tmp/test1-c.ll > /dev/null 2>> /output/csi.solution

# My answer for part1 
opt -load $LLVM_SO/submission_pt1.so -cse231-csi < /tmp/test1-c.ll > /dev/null 2> /output/csi.result
opt -load $LLVM_SO/submission_pt1.so -cse231-csi < /tmp/test1.ll > /dev/null 2>> /output/csi.result
opt -load $LLVM_SO/submission_pt1.so -cse231-csi < /tmp/test1-c.ll > /dev/null 2>> /output/csi.result
opt -load $LLVM_SO/submission_pt1.so -cse231-csi < /tmp/test1-c.ll > /dev/null 2>> /output/csi.result


# Testing for part2
opt -load $LLVM_SO/231_solution.so -cse231-cdi < /tmp/myTest.ll -o /tmp/mytest-cdi-solution.bc
clang++ /lib231/lib231.cpp $FLAGS /tmp/mytest-cdi-solution.bc -o /tmp/mytest-cdi-solution.bc

opt -load submission_pt1.so -cse231-cdi < /tmp/myTest.ll -o /tmp/mytest-cdi-answer.bc
clang++ /lib231/lib231.cpp $FLAGS /tmp/mytest-cdi-answer.bc -o part2Answer


# Testing for part 3:

#$LLVM_BIN/opt -load $LLVM_SO/C231_solution.so -cse231-bb < /tmp/test1.ll -o /tmp/test1-bb.bc

#$LLVM_BIN/llvm-dis /tmp/test1-cdi.bc
#$LLVM_BIN/llvm-dis /tmp/test1-bb.bc

#$LLVM_BIN/clang++ /tmp/lib231.ll /tmp/test1-cdi.ll /tmp/test1-main.ll -o /tmp/cdi_test1
#$LLVM_BIN/clang++ /tmp/lib231.ll /tmp/test1-bb.ll /tmp/test1-main.ll -o /tmp/bb_test1

#/tmp/cdi_test1 2> /tmp/cdi.result
#/tmp/bb_test1 2> /tmp/bb.result

