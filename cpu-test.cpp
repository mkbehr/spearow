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

int instr_daa() {
  // Test daa by actually trying to add and subtract integers. TODO:
  // figure out correct behavior for overflow here.
  for (int a = 0; a < 100; a++) {
    for (int b = 0; b < 100; b++) {
      // test addition
      {
        CPU cpu;
        cpu.rom.empty();
        cpu.rom.push_back(0x80); // ADD B
        cpu.rom.push_back(0x27); // DAA
        cpu.rom.push_back(0x00);
        cpu.pc = 0x0;
        int a_bcd = (a % 10) + (a / 10) * 0x10;
        cpu.af.high = (uint8_t) a_bcd;
        int b_bcd = (b % 10) + (b / 10) * 0x10;
        cpu.bc.high = (uint8_t) b_bcd;
        cpu.tick();
        cpu.tick();
        int sum = a+b;
        int sum_bcd = (sum % 10) + ((sum % 100) / 10) * 0x10;
        if (cpu.af.high != sum_bcd) {
          printf("DAA failed: tried BCD %02x + %02x, expected BCD %02x, got BCD %02x\n",
                 a_bcd, b_bcd, sum_bcd, cpu.af.high);
          return 0;
        }
      }
      // test subtraction
      if (a >= b) {
        CPU cpu;
        cpu.rom.empty();
        cpu.rom.push_back(0x90); // SUB B
        cpu.rom.push_back(0x27); // DAA
        cpu.pc = 0x0;
        int a_bcd = (a % 10) + (a / 10) * 0x10;
        cpu.af.high = (uint8_t) a_bcd;
        int b_bcd = (b % 10) + (b / 10) * 0x10;
        cpu.bc.high = (uint8_t) b_bcd;
        cpu.tick();
        cpu.tick();
        int diff = a-b;
        if (diff < 0) {
          diff += 100;
        }
        int diff_bcd = (diff % 10) + ((diff % 100) / 10) * 0x10;
        if (cpu.af.high != diff_bcd) {
          printf("DAA failed: tried BCD %02x - %02x, expected BCD %02x, got BCD %02x\n",
                 a_bcd, b_bcd, diff_bcd, cpu.af.high);
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
  int daa_pass = instr_daa();
  std::cout << "Test instr_daa: " <<
    (daa_pass ? "passed" : "failed") <<
    "\n";
  return 0;
}
