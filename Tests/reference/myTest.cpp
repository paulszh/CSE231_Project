#include <iostream>

int foo(volatile unsigned a) {
  volatile unsigned x = 10;
  
  if (a > 5)
    x = x + 5;
  else
    x = x + 50;
  
  return x+a;
}

int main() {
  foo (0);
  std::cerr << "==================== \n";
  foo (100);
  return 0;
}