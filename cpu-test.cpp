#include <iostream>

#include "cpu.hpp"

// TODO set up a proper test framework

int register_pair_union() {
  register_pair foo;

  foo.high = 0;
  foo.low = 10;

  return foo.full == 10;
}

int main() {
  std::cout << "Test register_pair_union: " <<
    (register_pair_union() ? "passed" : "failed") <<
    "\n";
  return 0;
}
