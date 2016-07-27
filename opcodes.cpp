#include <cstdint>
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

// Get address of 8-bit registers or memory locations in this order:
// B, D, H, (HL)
uint8_t *reg_8_high_or_indirect(CPU &cpu, int n) {
  switch (n) {
  case 0:
    return &cpu.bc.high;
  case 1:
    return &cpu.de.high;
  case 2:
    return &cpu.hl.high;
  case 3:
    return cpu.mem_ptr(cpu.hl.full);
  default:
    throw std::logic_error("Bad opcode argument bits");
  }
}

// Get address of 8-bit registers in this order:
// C, E, L, A
uint8_t *reg_8_low(CPU &cpu, int n) {
  switch (n) {
  case 0:
    return &cpu.bc.low;
  case 1:
    return &cpu.de.low;
  case 2:
    return &cpu.hl.low;
  case 3:
    return &cpu.af.high;
  default:
    throw std::logic_error("Bad opcode argument bits");
  }
}

// Get address of the nth 16-bit register, in order of: BC, DE, HL, SP
uint16_t *reg_16(CPU &cpu, int n) {
  switch (n) {
  case 0:
    return &cpu.bc.full;
  case 1:
    return &cpu.de.full;
  case 2:
    return &cpu.hl.full;
  case 3:
    return &cpu.sp;
  default:
    throw std::logic_error("Bad opcode argument bits");
  }
}

// Get indirect address from (BC), (DE), (HL+), or (HL-). In the case
// of (HL+) or (HL-), increment or decrement HL.
uint8_t *reg_16_deref_and_modify(CPU &cpu, int n) {
  uint16_t addr;
  switch (n) {
  case 0:
    addr = cpu.bc.full;
    break;
  case 1:
    addr = cpu.de.full;
    break;
  case 2:
    addr = cpu.hl.full;
    cpu.hl.full++;
    break;
  case 3:
    addr = cpu.hl.full;
    cpu.hl.full--;
    break;
  default:
    throw std::logic_error("Bad opcode argument bits");
  }
  return cpu.mem_ptr(addr);
}

int operate(CPU &cpu, unsigned char *op) {

  // Execute an opcode. Returns the number of machine cycles it took
  // (1 machine cycle = 4 clock cycles)

  unsigned char opcode = *op;

  int cycles;

  // Opcode table is divided into four regions by top two bits of opcode.
  switch (opcode & 0xC0) {
  case 0x00:

    // Various opcodes, divided into groups by bits 4 through 7
    switch (opcode & 0xf) {
    case 0:
      switch (opcode) {
      case 0x00: // 00: NOP
        return 1;
      case 0x10: // 10: STOP
        break;
      case 0x20: // 20: JR NZ,r8
        break;
      case 0x30: // 30: JR NC,r8
        break;
      default:
        throw std::logic_error("Bad opcode bits");
      }
      break;
    case 1: // 01, 11, 21, 31: 16-bit LD. 3 cycles. Flags unmodified.
    {
      uint16_t *arg = reg_16(cpu, (opcode >> 4) & 0x3);
      *arg = (op[1] << 8) + op[2];
      return 3;
    }
    case 2: // 02, 12, 22, 32: 8-bit LD from register A to indirect
            // address. 2 cycles. Flags unmodified. May increment or
            // decrement HL.
    {
      uint8_t *dst = reg_16_deref_and_modify(cpu, (opcode >> 4) & 0x3);
      *dst = cpu.af.high;
      return 2;
    }
    case 3: // 03, 13, 23, 33: 16-bit INC. 2 cycles. Flags unmodified.
    {
      uint16_t *arg = reg_16(cpu, (opcode >> 4) & 0x3);
      (*arg)++;
      return 2;
    }
    case 4: // 04, 14, 24, 34: 8-bit INC on high registers or (HL). 1
            // cycle, unless (HL) was dereferenced. Modifies flags Z, N, H.
    {
      uint8_t *arg = reg_8_high_or_indirect(cpu, (opcode >> 4) & 0x3);
      uint8_t result = *arg+1;
      int carryH = (result & 0x10) != (*arg & 0x10);
      *arg = result;
      cpu.updateFlags(!result, 0, carryH, -1);
      return (opcode == 0x34) ? 1 : 3;
    }
    case 5: // 05, 15, 25, 35: 8-bit DEC on high registers or (HL). 1
            // cycle, unless (HL) was dereferenced. Modifies flags Z, N, H.
    {
      uint8_t *arg = reg_8_high_or_indirect(cpu, (opcode >> 4) & 0x3);
      uint8_t result = *arg-1;
      int carryH = (result & 0x10) != (*arg & 0x10);
      *arg = result;
      cpu.updateFlags(!result, 1, carryH, -1);
      return (opcode == 0x35) ? 1 : 3;
    }
    case 6: // 06, 16, 26, 36: 8-bit immediate LD to high-registers or
            // (HL). 2 cycles, unless (HL) was dereferenced. Flags
            // unmodified.
    {
      uint8_t *dst = reg_8_high_or_indirect(cpu, (opcode >> 4) & 0x3);
      *dst = op[1];
      return (opcode == 0x36) ? 2 : 3;
    }
    case 7:
      switch (opcode) {
      case 0x07: // 07: RLCA. Rotate A left, MSB to carry flag and
                 // LSB. All other flags are unset. 1 cycle.
      {
        unsigned int rotated = cpu.af.high << 1;
        cpu.af.high = (rotated & 0xff) + (rotated >> 8); // rotate high bit to low bit
        // Note: There are some inconsistencies in documentation re
        // whether Z flag is always unset or set according to the
        // result. See https://hax.iimarck.us/post/12019/ for discussion.
        cpu.updateFlags(0, 0, 0, rotated >> 8);
        return 1;
      }
      case 0x17: // 17: RLA. Rotate A left through carry flag. All
                 // other flags are unset. 1 cycle.
      {
        unsigned int rotated = cpu.af.high << 1;
        cpu.af.high = (rotated & 0xff) + !!(cpu.af.low & FLAG_C); // rotate carry flag to low bit
        // Note: There are some inconsistencies in documentation re
        // whether Z flag is always unset or set according to the
        // result. See https://hax.iimarck.us/post/12019/ for discussion.
        cpu.updateFlags(0, 0, 0, rotated >> 8);
        return 1;
      }
      case 0x27: // DAA. Modify A so that if the previous instruction
                 // was an arithmetic operation, and its arguments are
                 // interpreted as binary-coded decimal (BCD) numbers,
                 // the value of A represents the BCD result. 1 cycle.
                 // Flags Z, H, and C modified.
      {
        // DAA algorithm taken from http://z80-heaven.wikidot.com/instructions-set:daa
        int newFlagC = 0;
        if (((cpu.af.high & 0xf) > 9) || (cpu.af.low & FLAG_H)) {
          cpu.af.high += 6;
        }
        if (((cpu.af.high & 0xf0) > 0x90) || (cpu.af.low & FLAG_C)) {
          newFlagC = 1;
          cpu.af.high += 0x60;
        }
        cpu.updateFlags(!cpu.af.high, -1, 0, newFlagC);
        return 1;
      }
      case 0x37: // SCF. Sets carry flag. 1 cycle, other flags unmodified.
        cpu.updateFlags(-1, -1, -1, 1);
        return 1;
      default:
        throw std::logic_error("Bad opcode");
      }
      throw std::logic_error("Exited switch statement"); // unreachable
    case 8: // TODO: LD or JR
      break;
    case 9: // 09, 19, 29, 39: 16-bit ADD to HL. Modifies all flags
            // but Z. 2 cycles.
    {
      uint16_t *arg = reg_16(cpu, (opcode >> 4) & 0x3);
      int result = cpu.hl.full + *arg;
      // For 16-bit arithmetic, carry flags pay attention to the high
      // byte.
      int carryH = (result & (1<<12)) !=
        ((cpu.hl.full & (1<<12)) ^ (*arg & (1<<12)));
      int carryC = !!(result & (1<<16));
      cpu.updateFlags(-1, 0, carryH, carryC);
      cpu.hl.full = result & 0xffff;
      return 2;
    }
    case 0xa: // 0a, 1a, 2a, 3a: 8-bit LD from indirect address to A.
              // 2 cycles. Flags unmodified. May increment or
              // decrement HL.
    {
      uint8_t *src = reg_16_deref_and_modify(cpu, (opcode >> 4) & 0x3);
      cpu.af.high = *src;
      return 2;
    }
    case 0xb: // 0b, 1b, 2b, 3b: 16-bit DEC. 2 cycles. Flags unmodified.
    {
      uint16_t *arg = reg_16(cpu, (opcode >> 4) & 0x3);
      (*arg)--;
      return 2;
    }
    case 0xc: // 0c, 1c, 2c, 3c: 8-bit INC on low registers or A. 1
              // cycle. Modifies flags Z, N, H.
    {
      uint8_t *arg = reg_8_low(cpu, (opcode >> 4) & 0x3);
      uint8_t result = *arg+1;
      int carryH = (result & 0x10) != (*arg & 0x10);
      *arg = result;
      cpu.updateFlags(!result, 0, carryH, -1);
      return 1;
    }
    case 0xd: // 0c, 1c, 2c, 3c: 8-bit INC on low registers or A. 1
              // cycle. Modifies flags Z, N, H.
    {
      uint8_t *arg = reg_8_low(cpu, (opcode >> 4) & 0x3);
      uint8_t result = *arg-1;
      int carryH = (result & 0x10) != (*arg & 0x10);
      *arg = result;
      cpu.updateFlags(!result, 1, carryH, -1);
      return 1;
    }
    case 0xe: // 0e, 1e, 2e, 3e: 8-bit immediate LD to low registers.
              // 2 cycles. Flags unmodified.
    {
      uint8_t *dst = reg_8_low(cpu, (opcode >> 4) & 0x3);
      *dst = op[1];
      return 2;
    }
    case 0xf:
      switch (opcode) {
      case 0x0f: // 0f: RRCA. Rotate A right, LSB to carry flag and
                 // MSB. All other flags are unset. 1 cycle.
      {
        unsigned int rotated = cpu.af.high >> 1;
        int flagC = (cpu.af.high & 1);
        cpu.af.high = rotated + (flagC << 7);
        cpu.updateFlags(0, 0, 0, flagC);
        return 1;
      }
      case 0x1f: // 1f: RRA. Rotate A right through carry flag. All
                 // other flags are unset. 1 cycle.
      {
        unsigned int rotated = cpu.af.high >> 1;
        int flagC = (cpu.af.high & 1);
        cpu.af.high = rotated + (!!(cpu.af.low & FLAG_C) << 7);
        cpu.updateFlags(0, 0, 0, flagC);
        return 1;
      }
      case 0x2f: // 2f: CPL. Complement A register. Flags N and H
                 // set. 1 cycle.
      {
        cpu.af.high = ~cpu.af.high;
        cpu.updateFlags(-1, 1, 1, -1);
        return 1;
      }
      case 0x3f: // 3f: Complement carry flag. Flags N and H unset. 1
                 // cycle.
      {
        cpu.updateFlags(-1, 0, 0, !(cpu.af.low & FLAG_C));
        return 1;
      }
      default:
        throw std::logic_error("Bad opcode");
      }
      throw std::logic_error("Exited switch statement"); // unreachable
    default:
      throw std::logic_error("Bad opcode low bits");
    }

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
      break;
    case 6: // (HL)
    {
      unsigned int addr = cpu.hl.full;
      cycles++;
      dst = cpu.mem_ptr(addr);
      break;
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
    {
      unsigned int addr = cpu.hl.full;
      cycles++;
      src = cpu.mem_ptr(addr);
      break;
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
    {
      unsigned int addr = cpu.hl.full;
      cycles++;
      arg = cpu.mem_ptr(addr);
      break;
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
