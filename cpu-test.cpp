#include <cstdio>
#include <iostream>

#include "cpu.hpp"

// TODO set up a proper test framework

int register_pair_union() {
  register_pair foo;

  foo.high = 0;
  foo.low = 10;

  return foo.full == 10;
}

int instr_sbc_imm() {
  for (int a = 0; a < 256; a++) {
    for (int d = 0; d < 256; d++) {
      for (int c = 0; c < 2; c++) {
        CPU cpu;
        cpu.rom.empty();
        cpu.rom.push_back(0xde);
        cpu.rom.push_back((uint8_t) d);
        cpu.pc = 0x0;
        cpu.af.high = (uint8_t) a;
        cpu.af.low = c * FLAG_C;
        cpu.tick();
        if (cpu.af.high != (uint8_t) (a - d - c)) {
          printf("SBC failed: tried %02x - %02x with carry %d, got %02x\n",
                 a, d, c, cpu.af.high);
          return 0;
        }
        int expectedZ = (a == (uint8_t) (d + c));
        int expectedN = 1;
        int expectedH = ((a & 0xf) < (d & 0xf) + c);
        int expectedC = (a < (d + c));
        uint8_t expectedFlags = (expectedZ * FLAG_Z
                                 + expectedN * FLAG_N
                                 + expectedH * FLAG_H
                                 + expectedC * FLAG_C);
        if (cpu.af.low != expectedFlags) {
          printf("SBC failed: tried %02x - %02x with carry %d\n",
                 a,d,c);
          printf("expected flags %02x ", expectedFlags);
          cpu.printFlags(expectedFlags);
          printf(", got flags %02x ", cpu.af.low);
          cpu.printFlags(cpu.af.low);
          printf("\n");
          return 0;
        }
      }
    }
  }
  return 1;
}

int main() {
  std::cout << "Test register_pair_union: " <<
    (register_pair_union() ? "passed" : "failed") <<
    "\n";
  int sbc_pass = instr_sbc_imm();
  std::cout << "Test instr_sbc_imm: " <<
    (sbc_pass ? "passed" : "failed") <<
    "\n";
  return 0;
}
