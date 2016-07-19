#include <cstdio>
#include <cstdlib>
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

  int cycles;

  // Opcode table is divided into four regions by top two bits of opcode.
  switch (opcode & 0xC0) {
  case 0x00:

    unimp(opcode);
    return 0;

  case 0x40:

    // 8-bit LD command. Length is 1 byte. Takes 1 machine cycle,
    // unless we need to dereference (HL), which takes an extra cycle.
    // Flags are unaffected. If opcode is OPC_HALT, operation is HALT
    // instead of LD.

    cycles = 1;

    if (opcode == OPC_HALT) {
      unimp(opcode);
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
    {
      unsigned int addr = cpu.hl.full;
      cycles++;
      illop(opcode);
      return 0;
    }
    case 7: // A
      dst = &cpu.af.high;
      break;
    default:
      throw std::logic_error("Bad opcode argument bits");
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
      break;
    case 6: // (HL)
      // TODO: look up value of HL register, convert to pointer into
      // gameboy's memory
    {
      unsigned int addr = cpu.hl.full;
      cycles++;
      illop(opcode);
      return 0;
    }
    case 7: // A
      src = &cpu.af.high;
      break;
    default:
      throw std::logic_error("Bad opcode argument bits");
    }

    *dst = *src;
    return cycles;

  case 0x80:

    // 8-bit arithmetic command. Length is 1 byte. Takes 1 machine
    // cycle unless we need to dereference (HL), which takes an extra
    // cycle. Flags are affected according to the operation.

    cycles = 1;

    unsigned char *arg;

    // Bits 2 through 4 determine argument
    switch ((opcode >> 3) & 0x7) {
    case 0: // B
      arg = &cpu.bc.high;
      break;
    case 1: // C
      arg = &cpu.bc.low;
      break;
    case 2: // D
      arg = &cpu.de.high;
      break;
    case 3: // E
      arg = &cpu.de.low;
      break;
    case 4: // H
      arg = &cpu.hl.high;
      break;
    case 5: // L
      arg = &cpu.hl.low;
      break;
    case 6: // (HL)
      // TODO: look up value of HL register, convert to pointer into
      // gameboy's memory
    {
      unsigned int addr = cpu.hl.full;
      cycles++;
      illop(opcode);
      return 0;
    }
    case 7: // A
      arg = &cpu.af.high;
      break;
    default:
      throw std::logic_error("Bad opcode argument bits");
    }

    // Bits 5 through 7 determine operation

    switch (opcode & 0x7) {
    case 0: // ADD A,arg
    {
      int result = cpu.af.high + *arg;
      // We carried from bit 3 iff bit 4 of the result isn't the same
      // as the XOR of bits 4 of the arguments
      int carryH = (result & (1<<4)) !=
        ((cpu.af.high & (1<<4)) ^ (*arg & (1<<4)));
      int carryC = !!(result & (1<<8));
      cpu.af.high = result & 0xff;
      cpu.updateFlags(!result, 0, carryH, carryC);
      break;
    }
    case 1: // ADC A,arg
    {
      int result = cpu.af.high + *arg + !!(cpu.af.low & FLAG_C);
      // We carried from bit 3 iff bit 4 of the result isn't the same
      // as the XOR of bits 4 of the arguments
      int carryH = (result & (1<<4)) !=
        ((cpu.af.high & (1<<4)) ^ (*arg & (1<<4)));
      int carryC = !!(result & (1<<8));
      cpu.af.high = result & 0xff;
      cpu.updateFlags(!result, 0, carryH, carryC);
      break;
    }
    case 2: // SUB arg
    {
      unsigned char subtractend = ~(*arg) + 1;
      int result = cpu.af.high + subtractend;
      int carryH = (result & (1<<4)) !=
        ((cpu.af.high & (1<<4)) ^ (subtractend & (1<<4)));
      int carryC = !!(result & (1<<8));
      cpu.af.high = result & 0xff;
      cpu.updateFlags(!result, 1, carryH, carryC);
      break;
    }
    case 3: // SBC arg
    {
      unsigned char subtractend = ~(*arg) + 1;
      int result = cpu.af.high + subtractend - !!(cpu.af.low & FLAG_C);
      int carryH = (result & (1<<4)) !=
        ((cpu.af.high & (1<<4)) ^ (subtractend & (1<<4)));
      int carryC = !!(result & (1<<8));
      cpu.af.high = result & 0xff;
      cpu.updateFlags(!result, 1, carryH, carryC);
      break;
    }
    case 4: // AND arg
    {
      unsigned char result = cpu.af.high & *arg;
      cpu.af.high = result;
      cpu.updateFlags(!result, 0, 1, 0);
      break;
    }
    case 5: // XOR arg
    {
      unsigned char result = cpu.af.high ^ *arg;
      cpu.af.high = result;
      cpu.updateFlags(!result, 0, 1, 0);
      break;
    }
    case 6: // OR arg
    {
      unsigned char result = cpu.af.high | *arg;
      cpu.af.high = result;
      cpu.updateFlags(!result, 0, 1, 0);
      break;
    }
    case 7: // CP arg
    {
      // Same as SUB, but don't change A
      unsigned char subtractend = ~(*arg) + 1;
      int result = cpu.af.high + subtractend;
      int carryH = (result & (1<<4)) !=
        ((cpu.af.high & (1<<4)) ^ (subtractend & (1<<4)));
      int carryC = !!(result & (1<<8));
      cpu.updateFlags(!result, 1, carryH, carryC);
      break;
    }
    default:
      throw std::logic_error("Bad opcode argument bits");
    }

    return cycles;

  case 0xC0:

    unimp(opcode);
    return 0;

  default:
    throw std::logic_error("Bad opcode high bits");
  }

}
