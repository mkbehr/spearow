#include <iostream>

#include "cpu.hpp"

// TODO set up a proper test framework

int register_pair_union() {
  register_pair foo;

  foo.small.high = 0;
  foo.small.low = 10;

  return foo.big == 10;
}

int main() {
  std::cout << "Test register_pair_union: " <<
    (register_pair_union() ? "passed" : "failed") <<
    "\n";
  return 0;
}
