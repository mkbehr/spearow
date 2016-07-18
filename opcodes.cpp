#include <cstdio>
#include <stdexcept>

#include "cpu.hpp"
#include "opcodes.hpp"

void illop(unsigned char opcode) {
  fprintf(stderr, "Illegal operation %02x\n", opcode);
  exit(0);
}

void unimp(unsigned char opcode) {
  fprintf(stderr, "Unimplemented operation %02x\n", opcode);
  exit(0);
}

int operate(CPU &cpu, unsigned char *op) {

  // Execute an opcode. Returns the number of machine cycles it took
  // (1 machine cycle = 4 clock cycles)

  unsigned char opcode = *op;

  // Opcode table is divided into four regions by top two bits of opcode.
  switch (opcode & 0xC0) {
  case 0x00:

    unimp();
    return 0;

  case 0x40:

    // 8-bit LD command. Length is 1 byte. Takes 1 machine cycle,
    // unless we need to dereference (HL), which takes an extra cycle.
    // Flags are unaffected. If opcode is OPC_HALT, operation is HALT
    // instead of LD.

    int cycles = 1;

    if (opcode == OPC_HALT) {
      unimp();
      return 0;
    }

    unsigned char (*src), (*dst);

    // Bits 2 through 4 determine the first argument (destination) (in
    // order of most-least significant, starting at 0)
    switch ((opcode >> 3) & 0x7) {
    case 0: // B
      dst = &cpu.bc.high;
      break;
    case 1: // C
      dst = &cpu.bc.low;
      break;
    case 2: // D
      dst = &cpu.de.high;
      break;
    case 3: // E
      dst = &cpu.de.low;
      break;
    case 4: // H
      dst = &cpu.hl.high;
      break;
    case 5: // L
      dst = &cpu.hl.low;
    case 6: // (HL)
      // TODO: look up value of HL register, convert to pointer into
      // gameboy's memory
      unsigned int addr = cpu.hl.full;
      cycles++;
      illop();
      return;
    case 7: // A
      dst = &cpu.af.high;
      break;
    default:
      throw std::logic_error("Bad opcode argument bits")
    }

    // Bits 5 through 7 determine second argument (source)
    switch (opcode & 0x7) {
    case 0: // B
      src = &cpu.bc.high;
      break;
    case 1: // C
      src = &cpu.bc.low;
      break;
    case 2: // D
      src = &cpu.de.high;
      break;
    case 3: // E
      src = &cpu.de.low;
      break;
    case 4: // H
      src = &cpu.hl.high;
      break;
    case 5: // L
      src = &cpu.hl.low;
    case 6: // (HL)
      // TODO: look up value of HL register, convert to pointer into
      // gameboy's memory
      unsigned int addr = cpu.hl.full;
      cycles++;
      illop();
      return;
    case 7: // A
      src = &cpu.af.high;
      break;
    default:
      throw std::logic_error("Bad opcode argument bits")
    }

    *dst = *src;
    return cycles;

  case 0x80:

    unimp();
    return 0;

  case 0xC0:

    unimp();
    return 0;
  }

}
