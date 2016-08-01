#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

#include "cpu.hpp"
#include "mem.hpp"
#include "opcodes.hpp"

const int OPCODE_LENGTHS[256] = {
  // 00-0f
  1, 3, 1, 1, 1, 1, 2, 1, 3, 1, 1, 1, 1, 1, 2, 1,
  // 10-1f
  2, 3, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1,
  // 20-2f
  2, 3, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1,
  // 30-3f
  2, 3, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1,
  // 40-4f
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  // 50-5f
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  // 60-6f
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  // 70-7f
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  // 80-8f
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  // 90-9f
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  // a0-af
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  // b0-bf
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  // c0-cf
  1, 1, 3, 3, 3, 1, 2, 1, 1, 1, 3, 2, 3, 3, 2, 1,
  // d0-df
  1, 1, 3, 0, 3, 1, 2, 1, 1, 1, 3, 0, 3, 0, 2, 1,
  // e0-ef
  2, 1, 2, 0, 0, 1, 2, 1, 2, 1, 3, 0, 0, 0, 2, 1,
  // f0-ff
  2, 1, 2, 1, 0, 1, 2, 1, 2, 1, 3, 1, 0, 0, 2, 1,
};

const char * OPCODE_NAMES[256] = {
  // 00-0f
  "NOP", "LD BC,d16", "LD (BC),A", "INC BC", "INC B", "DEC B", "LD B,d8", "RLCA",
  "LD (a16),SP", "ADD HL,BC", "LD A,(BC)", "DEC BC", "INC C", "DEC C", "LD C,d8", "RRCA",
  // 10-1f
  "STOP 0", "LD DE,d16", "LD (DE),A", "INC DE", "INC D", "DEC D", "LD D,d8", "RLA",
  "JR r8", "ADD HL,DE", "LD A,(DE)", "DEC DE", "INC E", "DEC E", "LD E,d8", "RRA",
  // 20-2f
  "JR NZ,r8", "LD HL,d16", "LD (HL+),A", "INC HL", "INC H", "DEC H", "LD H,d8", "DAA",
  "JR Z,r8", "ADD HL,HL", "LD A,(HL+)", "DEC HL", "INC L", "DEC L", "LD L,d8", "CPL",
  // 30-3f
  "JR NC,r8", "LD SP,d16", "LD (HL-),A", "INC SP", "INC (HL)", "DEC (HL)", "LD (HL),d8", "SCF",
  "JR C,r8", "ADD HL,SP", "LD A,(HL-)", "DEC SP", "INC A", "DEC A", "LD A,d8", "CCF",
  // 40-4f
  "LD B,B", "LD B,C", "LD B,D", "LD B,E", "LD B,H", "LD B,L", "LD B,(HL)", "LD B,A",
  "LD C,B", "LD C,C", "LD C,D", "LD C,E", "LD C,H", "LD C,L", "LD C,(HL)", "LD C,A",
  // 50-5f
  "LD D,B", "LD D,C", "LD D,D", "LD D,E", "LD D,H", "LD D,L", "LD D,(HL)", "LD D,A",
  "LD E,B", "LD E,C", "LD E,D", "LD E,E", "LD E,H", "LD E,L", "LD E,(HL)", "LD E,A",
  // 60-6f
  "LD H,B", "LD H,C", "LD H,D", "LD H,E", "LD H,H", "LD H,L", "LD H,(HL)", "LD H,A",
  "LD L,B", "LD L,C", "LD L,D", "LD L,E", "LD L,H", "LD L,L", "LD L,(HL)", "LD L,A",
  // 70-7f
  "LD (HL),B", "LD (HL),C", "LD (HL),D", "LD (HL),E", "LD (HL),H", "LD (HL),L", "HALT", "LD (HL),A",
  "LD A,B", "LD A,C", "LD A,D", "LD A,E", "LD A,H", "LD A,L", "LD A,(HL)", "LD A,A",
  // 80-8f
  "ADD A,B", "ADD A,C", "ADD A,D", "ADD A,E", "ADD A,H", "ADD A,L", "ADD A,(HL)", "ADD A,A",
  "ADC A,B", "ADC A,C", "ADC A,D", "ADC A,E", "ADC A,H", "ADC A,L", "ADC A,(HL)", "ADC A,A",
  // 90-9f
  "SUB B", "SUB C", "SUB D", "SUB E", "SUB H", "SUB L", "SUB (HL)", "SUB A",
  "SBC A,B", "SBC A,C", "SBC A,D", "SBC A,E", "SBC A,H", "SBC A,L", "SBC A,(HL)", "SBC A,A",
  // a0-af
  "AND B", "AND C", "AND D", "AND E", "AND H", "AND L", "AND (HL)", "AND A",
  "XOR B", "XOR C", "XOR D", "XOR E", "XOR H", "XOR L", "XOR (HL)", "XOR A",
  // b0-bf
  "OR B", "OR C", "OR D", "OR E", "OR H", "OR L", "OR (HL)", "OR A",
  "CP B", "CP C", "CP D", "CP E", "CP H", "CP L", "CP (HL)", "CP A",
  // c0-cf
  "RET NZ", "POP BC", "JP NZ,a16", "JP a16", "CALL NZ,a16", "PUSH BC", "ADD A,d8", "RST 00H",
  "RET Z", "RET", "JP Z,a16", "PREFIX CB", "CALL Z,a16", "CALL a16", "ADC A,d8", "RST 08H",
  // d0-df
  "RET NC", "POP DE", "JP NC,a16", "ILLOP", "CALL NC,a16", "PUSH DE", "SUB d8", "RST 10H",
  "RET C", "RETI", "JP C,a16", "ILLOP", "CALL C,a16", "ILLOP", "SBC A,d8", "RST 18H",
  // e0-ef
  "LDH (a8),A", "POP HL", "LD (C),A", "ILLOP", "ILLOP", "PUSH HL", "AND d8", "RST 20H",
  "ADD SP,r8", "JP (HL)", "LD (a16),A", "ILLOP", "ILLOP", "ILLOP", "XOR d8", "RST 28H",
  // f0-ff
  "LDH A,(a8)", "POP AF", "LD A,(C)", "DI", "ILLOP", "PUSH AF", "OR d8", "RST 30H",
  "LD HL,SP+r8", "LD SP,HL", "LD A,(a16)", "EI", "ILLOP", "ILLOP", "CP d8", "RST 28H",
};

const char *CB_OPCODE_NAMES[256] = {
  // 00-0f
  "RLC B", "RLC C", "RLC D", "RLC E", "RLC H", "RLC L", "RLC (HL)", "RLC A",
  "RRC B", "RRC C", "RRC D", "RRC E", "RRC H", "RRC L", "RRC (HL)", "RRC A",
  // 10-1f
  "RL B", "RL C", "RL D", "RL E", "RL H", "RL L", "RL (HL)", "RL A",
  "RR B", "RR C", "RR D", "RR E", "RR H", "RR L", "RR (HL)", "RR A",
  // 20-2f
  "SLA B", "SLA C", "SLA D", "SLA E", "SLA H", "SLA L", "SLA (HL)", "SLA A",
  "SRA B", "SRA C", "SRA D", "SRA E", "SRA H", "SRA L", "SRA (HL)", "SRA A",
  // 30-3f
  "SWAP B", "SWAP C", "SWAP D", "SWAP E", "SWAP H", "SWAP L", "SWAP (HL)", "SWAP A",
  "SRL B", "SRL C", "SRL D", "SRL E", "SRL H", "SRL L", "SRL (HL)", "SRL A",
  // 40-4f
  "BIT 0,B", "BIT 0,C", "BIT 0,D", "BIT 0,E", "BIT 0,H", "BIT 0,L", "BIT 0,(HL)", "BIT 0,A",
  "BIT 1,B", "BIT 1,C", "BIT 1,D", "BIT 1,E", "BIT 1,H", "BIT 1,L", "BIT 1,(HL)", "BIT 1,A",
  // 50-5f
  "BIT 2,B", "BIT 2,C", "BIT 2,D", "BIT 2,E", "BIT 2,H", "BIT 2,L", "BIT 2,(HL)", "BIT 2,A",
  "BIT 3,B", "BIT 3,C", "BIT 3,D", "BIT 3,E", "BIT 3,H", "BIT 3,L", "BIT 3,(HL)", "BIT 3,A",
  // 60-6f
  "BIT 4,B", "BIT 4,C", "BIT 4,D", "BIT 4,E", "BIT 4,H", "BIT 4,L", "BIT 4,(HL)", "BIT 4,A",
  "BIT 5,B", "BIT 5,C", "BIT 5,D", "BIT 5,E", "BIT 5,H", "BIT 5,L", "BIT 5,(HL)", "BIT 5,A",
  // 70-7f
  "BIT 6,B", "BIT 6,C", "BIT 6,D", "BIT 6,E", "BIT 6,H", "BIT 6,L", "BIT 6,(HL)", "BIT 6,A",
  "BIT 7,B", "BIT 7,C", "BIT 7,D", "BIT 7,E", "BIT 7,H", "BIT 7,L", "BIT 7,(HL)", "BIT 7,A",
  // 80-8f
  "RES 0,B", "RES 0,C", "RES 0,D", "RES 0,E", "RES 0,H", "RES 0,L", "RES 0,(HL)", "RES 0,A",
  "RES 1,B", "RES 1,C", "RES 1,D", "RES 1,E", "RES 1,H", "RES 1,L", "RES 1,(HL)", "RES 1,A",
  // 90-9f
  "RES 2,B", "RES 2,C", "RES 2,D", "RES 2,E", "RES 2,H", "RES 2,L", "RES 2,(HL)", "RES 2,A",
  "RES 3,B", "RES 3,C", "RES 3,D", "RES 3,E", "RES 3,H", "RES 3,L", "RES 3,(HL)", "RES 3,A",
  // a0-af
  "RES 4,B", "RES 4,C", "RES 4,D", "RES 4,E", "RES 4,H", "RES 4,L", "RES 4,(HL)", "RES 4,A",
  "RES 5,B", "RES 5,C", "RES 5,D", "RES 5,E", "RES 5,H", "RES 5,L", "RES 5,(HL)", "RES 5,A",
  // b0-bf
  "RES 6,B", "RES 6,C", "RES 6,D", "RES 6,E", "RES 6,H", "RES 6,L", "RES 6,(HL)", "RES 6,A",
  "RES 7,B", "RES 7,C", "RES 7,D", "RES 7,E", "RES 7,H", "RES 7,L", "RES 7,(HL)", "RES 7,A",
  // c0-cf
  "SET 0,B", "SET 0,C", "SET 0,D", "SET 0,E", "SET 0,H", "SET 0,L", "SET 0,(HL)", "SET 0,A",
  "SET 1,B", "SET 1,C", "SET 1,D", "SET 1,E", "SET 1,H", "SET 1,L", "SET 1,(HL)", "SET 1,A",
  // d0-df
  "SET 2,B", "SET 2,C", "SET 2,D", "SET 2,E", "SET 2,H", "SET 2,L", "SET 2,(HL)", "SET 2,A",
  "SET 3,B", "SET 3,C", "SET 3,D", "SET 3,E", "SET 3,H", "SET 3,L", "SET 3,(HL)", "SET 3,A",
  // e0-ef
  "SET 4,B", "SET 4,C", "SET 4,D", "SET 4,E", "SET 4,H", "SET 4,L", "SET 4,(HL)", "SET 4,A",
  "SET 5,B", "SET 5,C", "SET 5,D", "SET 5,E", "SET 5,H", "SET 5,L", "SET 5,(HL)", "SET 5,A",
  // f0-ff
  "SET 6,B", "SET 6,C", "SET 6,D", "SET 6,E", "SET 6,H", "SET 6,L", "SET 6,(HL)", "SET 6,A",
  "SET 7,B", "SET 7,C", "SET 7,D", "SET 7,E", "SET 7,H", "SET 7,L", "SET 7,(HL)", "SET 7,A",
};

void illop(uint8_t opcode) {
  fprintf(stderr, "Illegal operation %02x\n", opcode);
  exit(0);
}

void unimp(uint8_t opcode) {
  fprintf(stderr, "Unimplemented operation %02x\n", opcode);
  exit(0);
}

// TODO: consider changing all the foo_or_indirect functions to
// increment a value if they did the indirect

// Get pointer to 8-bit registers or memory locations in this order:
// B, D, H, (HL)
gb_ptr reg_8_high_or_indirect(CPU &cpu, int n) {
  switch (n) {
  case 0:
    return gb_reg_ptr(cpu, &cpu.bc.high);
  case 1:
    return gb_reg_ptr(cpu, &cpu.de.high);
  case 2:
    return gb_reg_ptr(cpu, &cpu.hl.high);
  case 3:
    return gb_mem_ptr(cpu, cpu.hl.full);
  default:
    throw std::logic_error("Bad opcode argument bits");
  }
}

// Get pointer to 8-bit registers in this order:
// C, E, L, A
gb_ptr reg_8_low(CPU &cpu, int n) {
  switch (n) {
  case 0:
    return gb_reg_ptr(cpu, &cpu.bc.low);
  case 1:
    return gb_reg_ptr(cpu, &cpu.de.low);
  case 2:
    return gb_reg_ptr(cpu, &cpu.hl.low);
  case 3:
    return gb_reg_ptr(cpu, &cpu.af.high);
  default:
    throw std::logic_error("Bad opcode argument bits");
  }
}

// Get pointer to the nth 16-bit register, in order of: BC, DE, HL, SP
gb_ptr_16 reg_16_or_sp(CPU &cpu, int n) {
  switch (n) {
  case 0:
    return gb_reg16_ptr(cpu, &cpu.bc.full);
  case 1:
    return gb_reg16_ptr(cpu, &cpu.de.full);
  case 2:
    return gb_reg16_ptr(cpu, &cpu.hl.full);
  case 3:
    return gb_reg16_ptr(cpu, &cpu.sp);
  default:
    throw std::logic_error("Bad opcode argument bits");
  }
}

// Get address of the nth 16-bit register, in order of: BC, DE, HL, AF
gb_ptr_16 reg_16_or_af(CPU &cpu, int n) {
  switch (n) {
  case 0:
    return gb_reg16_ptr(cpu, &cpu.bc.full);
  case 1:
    return gb_reg16_ptr(cpu, &cpu.de.full);
  case 2:
    return gb_reg16_ptr(cpu, &cpu.hl.full);
  case 3:
    return gb_reg16_ptr(cpu, &cpu.af.full);
  default:
    throw std::logic_error("Bad opcode argument bits");
  }
}

// Get indirect address from (BC), (DE), (HL+), or (HL-). In the case
// of (HL+) or (HL-), increment or decrement HL.
gb_ptr reg_16_deref_and_modify(CPU &cpu, int n) {
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
  return gb_mem_ptr(cpu, addr);
}

// Get address of register or memory in this order: B, C, D, E, H, L,
// (HL), A. If we dereferenced (HL), increment the value at
// did_indirect.
gb_ptr reg_8_all_or_indirect(CPU &cpu, int n, int &did_indirect) {
  switch (n) {
  case 0:
    return gb_reg_ptr(cpu, &cpu.bc.high);
  case 1:
    return gb_reg_ptr(cpu, &cpu.bc.low);
  case 2:
    return gb_reg_ptr(cpu, &cpu.de.high);
  case 3:
    return gb_reg_ptr(cpu, &cpu.de.low);
  case 4:
    return gb_reg_ptr(cpu, &cpu.hl.high);
  case 5:
    return gb_reg_ptr(cpu, &cpu.hl.low);
  case 6:
    did_indirect++;
    return gb_mem_ptr(cpu, cpu.hl.full);
  case 7:
    return gb_reg_ptr(cpu, &cpu.af.high);
  default:
    throw std::logic_error("Bad opcode argument bits");
  }
}

inline void op_call(CPU &cpu, uint16_t call_addr) {
  // CALL operations (conditional or unconditional) are all 3
  // bytes long.
  uint16_t op_size = 3;
  uint16_t return_addr = cpu.pc + op_size;
  cpu.stack_push_16(return_addr);
  cpu.next_pc = call_addr;
}

inline void op_ret(CPU &cpu) {
  cpu.next_pc = cpu.stack_pop_16();
}

int operate(CPU &cpu, gb_ptr op) {

  // Execute an opcode. Returns the number of machine cycles it took
  // (1 machine cycle = 4 clock cycles)

  uint8_t opcode = op.read();

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
        cpu.stop();
        return 1;
      case 0x20: // 20: JR NZ,r8. 3 cycles if we jump, 2 otherwise.
                 // Flags unmodified.
        if (cpu.af.low & FLAG_Z) {
          return 2;
        } else {
          // JR operations are all 2 bytes long.
          uint16_t op_size = 2;
          // JR interprets its arg as a signed int.
          cpu.next_pc = cpu.pc + op_size + (int8_t) (op+1).read();
          return 3;
        }
      case 0x30: // 30: JR NC,r8. 3 cycles if we jump, 2 otherwise.
                 // Flags unmodified.
        if (cpu.af.low & FLAG_C) {
          return 2;
        } else {
          uint16_t op_size = 2;
          cpu.next_pc = cpu.pc + op_size + (int8_t) (op+1).read();
          return 3;
        }
      default:
        throw std::logic_error("Bad opcode bits");
      }
      break;
    case 1: // 01, 11, 21, 31: 16-bit LD. 3 cycles. Flags unmodified.
    {
      gb_ptr_16 arg = reg_16_or_sp(cpu, (opcode >> 4) & 0x3);
      // Z80 (and therefore gameboy is little-endian)
      arg.write((op+1).read_16());
      return 3;
    }
    case 2: // 02, 12, 22, 32: 8-bit LD from register A to indirect
            // address. 2 cycles. Flags unmodified. May increment or
            // decrement HL.
    {
      gb_ptr dst = reg_16_deref_and_modify(cpu, (opcode >> 4) & 0x3);
      dst.write(cpu.af.high);
      return 2;
    }
    case 3: // 03, 13, 23, 33: 16-bit INC. 2 cycles. Flags unmodified.
    {
      gb_ptr_16 arg = reg_16_or_sp(cpu, (opcode >> 4) & 0x3);
      arg.write(arg.read()+1);
      return 2;
    }
    case 4: // 04, 14, 24, 34: 8-bit INC on high registers or (HL). 1
            // cycle, unless (HL) was dereferenced. Modifies flags Z, N, H.
    {
      gb_ptr arg = reg_8_high_or_indirect(cpu, (opcode >> 4) & 0x3);
      uint8_t result = arg.read()+1;
      int carryH = (result & 0x10) != (arg.read() & 0x10);
      arg.write(result);
      cpu.updateFlags(!result, 0, carryH, -1);
      return (opcode == 0x34) ? 1 : 3;
    }
    case 5: // 05, 15, 25, 35: 8-bit DEC on high registers or (HL). 1
            // cycle, unless (HL) was dereferenced. Modifies flags Z, N, H.
    {
      gb_ptr arg = reg_8_high_or_indirect(cpu, (opcode >> 4) & 0x3);
      uint8_t result = arg.read()-1;
      int carryH = (result & 0x10) != (arg.read() & 0x10);
      arg.write(result);
      cpu.updateFlags(!result, 1, carryH, -1);
      return (opcode == 0x35) ? 1 : 3;
    }
    case 6: // 06, 16, 26, 36: 8-bit immediate LD to high-registers or
            // (HL). 2 cycles, unless (HL) was dereferenced. Flags
            // unmodified.
    {
      gb_ptr dst = reg_8_high_or_indirect(cpu, (opcode >> 4) & 0x3);
      dst.write((op+1).read());
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
    case 8:
      switch (opcode) {
      case 0x08: // 08: 16-bit load from SP to indirect. 5 cycles,
                 // flags unmodified.
      {
        uint16_t addr = (op+1).read_16();
        gb_ptr_16 dst = gb_mem16_ptr(cpu, addr);
        dst.write(cpu.sp);
        return 5;
      }
      case 0x18: // 18: unconditional JR. 3 cycles, flags
                 // unmodified.
      {
        uint16_t op_size = 2;
        cpu.next_pc = cpu.pc + op_size + (int8_t) (op+1).read();
        return 3;
      }
      case 0x28: // 28: JR Z
        if (cpu.af.low & FLAG_Z) {
          uint16_t op_size = 2;
          cpu.next_pc = cpu.pc + op_size + (int8_t) (op+1).read();
          return 3;
        } else {
          return 2;
        }
      case 0x38: // 38: JR C
        if (cpu.af.low & FLAG_C) {
          uint16_t op_size = 2;
          cpu.next_pc = cpu.pc + op_size + (int8_t) (op+1).read();
          return 3;
        } else {
          return 2;
        }
      default:
        throw std::logic_error("Bad opcode bits");
      }
      break;
    case 9: // 09, 19, 29, 39: 16-bit ADD to HL. Modifies all flags
            // but Z. 2 cycles.
    {
      gb_ptr_16 arg = reg_16_or_sp(cpu, (opcode >> 4) & 0x3);
      int result = cpu.hl.full + arg.read();
      // For 16-bit arithmetic, carry flags pay attention to the high
      // byte.
      int carryH = (result & (1<<12)) !=
        ((cpu.hl.full & (1<<12)) ^ (arg.read() & (1<<12)));
      int carryC = !!(result & (1<<16));
      cpu.updateFlags(-1, 0, carryH, carryC);
      cpu.hl.full = result & 0xffff;
      return 2;
    }
    case 0xa: // 0a, 1a, 2a, 3a: 8-bit LD from indirect address to A.
              // 2 cycles. Flags unmodified. May increment or
              // decrement HL.
    {
      gb_ptr src = reg_16_deref_and_modify(cpu, (opcode >> 4) & 0x3);
      cpu.af.high = src.read();
      return 2;
    }
    case 0xb: // 0b, 1b, 2b, 3b: 16-bit DEC. 2 cycles. Flags unmodified.
    {
      gb_ptr_16 arg = reg_16_or_sp(cpu, (opcode >> 4) & 0x3);
      arg.write(arg.read()-1);
      return 2;
    }
    case 0xc: // 0c, 1c, 2c, 3c: 8-bit INC on low registers or A. 1
              // cycle. Modifies flags Z, N, H.
    {
      gb_ptr arg = reg_8_low(cpu, (opcode >> 4) & 0x3);
      uint8_t result = arg.read()+1;
      int carryH = (result & 0x10) != (arg.read() & 0x10);
      arg.write(result);
      cpu.updateFlags(!result, 0, carryH, -1);
      return 1;
    }
    case 0xd: // 0c, 1c, 2c, 3c: 8-bit INC on low registers or A. 1
              // cycle. Modifies flags Z, N, H.
    {
      gb_ptr arg = reg_8_low(cpu, (opcode >> 4) & 0x3);
      uint8_t result = arg.read()-1;
      int carryH = (result & 0x10) != (arg.read() & 0x10);
      arg.write(result);
      cpu.updateFlags(!result, 1, carryH, -1);
      return 1;
    }
    case 0xe: // 0e, 1e, 2e, 3e: 8-bit immediate LD to low registers.
              // 2 cycles. Flags unmodified.
    {
      gb_ptr dst = reg_8_low(cpu, (opcode >> 4) & 0x3);
      dst.write((op+1).read());
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
    throw std::logic_error("Exited switch statement"); // unreachable

  case 0x40:
  {

    // 8-bit LD command. Length is 1 byte. Takes 1 machine cycle,
    // unless we need to dereference (HL), which takes an extra cycle.
    // Flags are unaffected. If opcode is OPC_HALT, operation is HALT
    // instead of LD.

    int cycles = 1;

    if (opcode == OPC_HALT) { // 76: HALT. 1 cycle, flags unaffected.
      cpu.halt();
      return 1;
    }

    // Bits 2 through 4 determine the first argument (destination) (in
    // order of most-least significant, starting at 0)
    gb_ptr dst = reg_8_all_or_indirect(cpu, (opcode >> 3) & 0x7, cycles);

    // Bits 5 through 7 determine second argument (source)
    gb_ptr src = reg_8_all_or_indirect(cpu, opcode & 0x7, cycles);

    dst.write(src.read());

    // reg_8_all_or_indirect has incremented cycles as necessary if we
    // had to dereference HL.
    return cycles;
  }
  case 0x80:
  {
    // 8-bit arithmetic command. Length is 1 byte. Takes 1 machine
    // cycle unless we need to dereference (HL), which takes an extra
    // cycle. Flags are affected according to the operation.

    int cycles = 1;

    // Bits 5 through 7 determine argument
    gb_ptr arg = reg_8_all_or_indirect(cpu, opcode & 0x7, cycles);

    // Bits 2 through 4 determine operation
    switch ((opcode>>3) & 0x7) {
    case 0: // ADD A,arg
    {
      int result = cpu.af.high + arg.read();
      // We carried from bit 3 iff bit 4 of the result isn't the same
      // as the XOR of bits 4 of the arguments
      int carryH = (result & (1<<4)) !=
        ((cpu.af.high & (1<<4)) ^ (arg.read() & (1<<4)));
      int carryC = !!(result & (1<<8));
      cpu.af.high = result & 0xff;
      cpu.updateFlags(!result, 0, carryH, carryC);
      break;
    }
    case 1: // ADC A,arg
    {
      int result = cpu.af.high + arg.read() + !!(cpu.af.low & FLAG_C);
      // We carried from bit 3 iff bit 4 of the result isn't the same
      // as the XOR of bits 4 of the arguments
      int carryH = (result & (1<<4)) !=
        ((cpu.af.high & (1<<4)) ^ (arg.read() & (1<<4)));
      int carryC = !!(result & (1<<8));
      cpu.af.high = result & 0xff;
      cpu.updateFlags(!result, 0, carryH, carryC);
      break;
    }
    case 2: // SUB arg
    {
      uint8_t subtractend = ~(arg.read()) + 1;
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
      uint8_t subtractend = ~(arg.read()) + 1;
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
      uint8_t result = cpu.af.high & arg.read();
      cpu.af.high = result;
      cpu.updateFlags(!result, 0, 1, 0);
      break;
    }
    case 5: // XOR arg
    {
      uint8_t result = cpu.af.high ^ arg.read();
      cpu.af.high = result;
      cpu.updateFlags(!result, 0, 1, 0);
      break;
    }
    case 6: // OR arg
    {
      uint8_t result = cpu.af.high | arg.read();
      cpu.af.high = result;
      cpu.updateFlags(!result, 0, 1, 0);
      break;
    }
    case 7: // CP arg
    {
      // Same as SUB, but don't change A
      uint8_t subtractend = ~(arg.read()) + 1;
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
  }
  case 0xC0:

    switch (opcode & 0xf) {
    case 0x0:
      switch (opcode) {
      case 0xC0: // C0: RET NZ: return if Z flag unset. 2 or 5 cycles,
                 // flags unmodified.
      {
        if (cpu.af.low & FLAG_Z) {
          return 2;
        } else {
          op_ret(cpu);
          return 5;
        }
      }
      case 0xD0: // D0: RET NC: return if C flag unset. 2 or 5 cycles,
                 // flags unmodified.
        {
        if (cpu.af.low & FLAG_C) {
          return 2;
        } else {
          op_ret(cpu);
          return 5;
        }
      }
      case 0xE0: // E0: LDH (a8), A. Writes contents of A to 0xff00
                 // plus argument. 3 cycles, flags unmodified.
      {
        uint16_t addr = 0xff00 + (op+1).read();
        gb_ptr ptr = gb_mem_ptr(cpu, addr);
        ptr.write(cpu.af.high);
        return 3;
      }
      case 0xF0: // F0: LDH A,(a8). Reads contents of 0xff00 plus
                 // argument into A. 3 cycles, flags unmodified.
      {
        uint16_t addr = 0xff00 + (op+1).read();
        gb_ptr ptr = gb_mem_ptr(cpu, addr);
        cpu.af.high = ptr.read();
        return 3;
      }
      default:
        throw std::logic_error("Bad opcode bits");
      }
      break;
    case 0x1: // C1, D1, E1, F1: 16-bit POP to BC, DE, HL, or AF.
              // Flags unmodified, unless of course we popped into AF.
              // 3 cycles.
    {
      uint16_t val = cpu.stack_pop_16();
      gb_ptr_16 ptr = reg_16_or_af(cpu, (opcode >> 4) & 0x3);
      ptr.write(val);
      return 3;
    }
    case 0x2:
      switch (opcode) {
      case 0xC2: // Absolute jump if NZ. 3 or 4 cycles, flags unmodified.
      {
        if (cpu.af.low & FLAG_Z) {
          return 3;
        } else {
          cpu.next_pc = (op+1).read_16();
          return 4;
        }
      }
      case 0xD2: // Absolute jump if NC. 3 or 4 cycles, flags unmodified.
      {
        if (cpu.af.low & FLAG_C) {
          return 3;
        } else {
          cpu.next_pc = (op+1).read_16();
          return 4;
        }
      }
      case 0xE2: // Write A to 0xff00 plus C. 2 cycles, flags unmodified.
      {
        uint16_t addr = 0xff00 + cpu.bc.low;
        gb_ptr ptr = gb_mem_ptr(cpu, addr);
        ptr.write(cpu.af.high);
        return 2;
      }
      case 0xF2: // Read 0xff00 plus C to A. 2 cycles, flags unmodified.
      {
        uint16_t addr = 0xff00 + cpu.bc.low;
        gb_ptr ptr = gb_mem_ptr(cpu, addr);
        cpu.af.high = ptr.read();
        return 2;
      }
      default:
        throw std::logic_error("Bad opcode bits");
      }
      break;
    case 0x3:
      switch (opcode) {
      case 0xC3: // Unconditional absolute jump. 4 cycles.
      {
        cpu.next_pc = (op+1).read_16();
        return 4;
      }
      case 0xD3: // both illegal
      case 0xE3:
        illop(opcode);
        return 0;
      case 0xF3: // Disable interrupts (after next operation). 1 cycle,
                 // flags unmodified.
      {
        cpu.disableInterrupts();
        return 1;
      }
      default:
        throw std::logic_error("Bad opcode bits");
      }
    case 0x4:
      switch (opcode) {
      case 0xC4: // Absolute call, conditional on NZ. 3 or 6 cycles,
                 // flags unmodified.
      {
        if (cpu.af.low & FLAG_Z) {
          return 3;
        } else {
          uint16_t call_addr = (op+1).read_16();
          op_call(cpu, call_addr);
          return 6;
        }
      }
      case 0xD4: // Absolute call, conditional on NC. 3 or 6 cycles,
                 // flags unmodified.
      {
        if (cpu.af.low & FLAG_Z) {
          return 3;
        } else {
          uint16_t call_addr = (op+1).read_16();
          op_call(cpu, call_addr);
          return 6;
        }
      }
      case 0xE4: // both illegal
      case 0xF4:
        illop(opcode);
        return 0;
      default:
        throw std::logic_error("Bad opcode bits");
      }
      break;
    case 0x5: // 16-bit PUSH from BC, DE, HL, or AF. 4 cycles, flags
              // unmodified.
    {
      gb_ptr_16 ptr = reg_16_or_af(cpu, (opcode >> 4) & 0x3);
      uint16_t val = ptr.read();
      cpu.stack_push_16(val);
      return 4;
    }
    case 0x6: // Some 8-bit math we didn't get to earlier. Always 2
              // cycles, modifies all flags.
      switch (opcode) {
      case 0xC6: // ADD A,d8
      {
        int result = cpu.af.high + (op+1).read();
        // We carried from bit 3 iff bit 4 of the result isn't the same
        // as the XOR of bits 4 of the arguments
        int carryH = (result & (1<<4)) !=
          ((cpu.af.high & (1<<4)) ^ ((op+1).read() & (1<<4)));
        int carryC = !!(result & (1<<8));
        cpu.af.high = result & 0xff;
        cpu.updateFlags(!result, 0, carryH, carryC);
        return 2;
      }
      case 0xD6: // SUB d8
      {
        uint8_t subtractend = ~((op+1).read()) + 1;
        int result = cpu.af.high + subtractend;
        int carryH = (result & (1<<4)) !=
          ((cpu.af.high & (1<<4)) ^ (subtractend & (1<<4)));
        int carryC = !!(result & (1<<8));
        cpu.af.high = result & 0xff;
        cpu.updateFlags(!result, 1, carryH, carryC);
        return 2;
      }
      case 0xE6: // AND d8
      {
        uint8_t result = cpu.af.high & (op+1).read();
        cpu.af.high = result;
        cpu.updateFlags(!result, 0, 1, 0);
        return 2;
      }
      case 0xF6: // OR d8
      {
        uint8_t result = cpu.af.high | (op+1).read();
        cpu.af.high = result;
        cpu.updateFlags(!result, 0, 1, 0);
        return 2;
      }
      default:
        throw std::logic_error("Bad opcode bits");
      }
      break;
    case 0x7: // RST: address onto stack, jump to fixed address. 4
              // cycles, flags unmodified.
    {
      // AFAICT, this should push the address of the /next/
      // instruction. Documentation is unclear.
      uint16_t op_size = 3;
      uint16_t return_addr = cpu.pc + op_size;
      cpu.stack_push_16(return_addr);

      uint16_t call_addr = 0 + (opcode & 0x30);
      cpu.next_pc = call_addr;

      return 4;
    }
    case 0x8:
      switch (opcode) {
      case 0xC8: // Conditional RET if Z flag set. 2 or 5 cycles,
                 // flags unmodified.
      {
        if (cpu.af.low & FLAG_Z) {
          op_ret(cpu);
          return 5;
        } else {
          return 2;
        }
      }
      case 0xD8: // Conditional RET if C flag set. 2 or 5 cycles,
                 // flags unmodified.
      {
        if (cpu.af.low & FLAG_C) {
          op_ret(cpu);
          return 5;
        } else {
          return 2;
        }
      }
      case 0xE8: // 16-bit add of 8-bit literal to stack pointer. 4
                 // cycles. Note that flag Z is always unset.
      {
        int8_t arg = (int8_t) (op+1).read();
        int result = cpu.sp + arg;
        // For 16-bit arithmetic, carry flags pay attention to the high
        // byte.
        int carryH = (result & (1<<12)) !=
          ((cpu.sp & (1<<12)) ^ (arg & (1<<12)));
        int carryC = !!(result & (1<<16));
        cpu.updateFlags(0, 0, carryH, carryC);
        cpu.sp = result & 0xffff;
        return 4;
      }
      case 0xF8: // Add 8-bit literal to stack pointer, store in HL. 3
                 // cycles. Flags set according to the addition, as in
                 // op E8 (I think).
      {
        int8_t arg = (int8_t) (op+1).read();
        int result = cpu.sp + arg;
        // For 16-bit arithmetic, carry flags pay attention to the high
        // byte.
        int carryH = (result & (1<<12)) !=
          ((cpu.sp & (1<<12)) ^ (arg & (1<<12)));
        int carryC = !!(result & (1<<16));
        cpu.updateFlags(0, 0, carryH, carryC);
        cpu.hl.full = result & 0xffff;
        return 3;
      }
      default:
        throw std::logic_error("Bad opcode bits");
      }
      break;
    case 0x9:
      switch (opcode) {
      case 0xC9: // RET: Unconditional return. 4 cycles.
      {
        op_ret(cpu);
        return 4;
      }
      case 0xD9: // RETI: Unconditional return and enable interrupts.
                 // Is the timing the same as the EI instruction?
                 // Unclear.
      {
        op_ret(cpu);
        cpu.enableInterrupts();
        return 4;
      }
      case 0xE9: // JP HL: Jump to address in HL. I usually see it
                 // written JP (HL) but that seems deceptive because I
                 // don't think we're actually looking up anything
                 // from memory. 1 cycle, flags unmodified.
      {
        cpu.next_pc = cpu.hl.full;
        return 1;
      }
      case 0xF9: // LD SP,HL: Move HL into stack pointer. 2 cycles,
                 // flags unmodified.
      {
        cpu.sp = cpu.hl.full;
        return 2;
      }
      default:
        throw std::logic_error("Bad opcode bits");
      }
      break;
    case 0xa:
      switch (opcode) {
      case 0xCA: // Absolute jump if Z. 3 or 4 cycles, flags unmodified.
      {
        if (cpu.af.low & FLAG_Z) {
          // Low byte first.
          cpu.next_pc = (op+1).read() + ((op+2).read() << 8);
          return 4;
        } else {
          return 3;
        }
      }
      case 0xDA: // Absolute jump if C. 3 or 4 cycles, flags unmodified.
      {
        if (cpu.af.low & FLAG_C) {
          // Low byte first.
          cpu.next_pc = (op+1).read() + ((op+2).read() << 8);
          return 4;
        } else {
          return 3;
        }
      }
      case 0xEA: // Write A to provided address. 4 cycles, flags unmodified.
      {
        uint16_t addr = (op+1).read() + ((op+2).read() << 8);
        gb_ptr ptr = gb_mem_ptr(cpu, addr);
        ptr.write(cpu.af.high);
        return 4;
      }
      case 0xFA: // Read provided address to A. 4 cycles, flags unmodified.
      {
        uint16_t addr = (op+1).read() + ((op+2).read() << 8);
        gb_ptr ptr = gb_mem_ptr(cpu, addr);
        cpu.af.high = ptr.read();
        return 4;
      }
      default:
        throw std::logic_error("Bad opcode bits");
      }
      break;
    case 0xb:
      switch (opcode) {
      case 0xCB: // Various math functions. Some modify some flags.
                 // All 2 cycles.
      {
        return cb_prefix_operate(cpu, (op+1).read());
      }
      case 0xDB: // both illegal
      case 0xEB:
        illop(opcode);
        return 0;
      case 0xFB: // Enable interrupts (after next operation). 1 cycle,
                 // flags unmodified.
      {
        cpu.enableInterrupts();
        return 1;
      }
      default:
        throw std::logic_error("Bad opcode bits");
      }
      break;
    case 0xc:
      switch (opcode) {
      case 0xCC: // Absolute call, conditional on Z. 3 or 6 cycles,
                 // flags unmodified.
      {
        if (cpu.af.low & FLAG_Z) {
          uint16_t call_addr = (op+1).read_16();
          op_call(cpu, call_addr);
          return 6;
        } else {
          return 3;
        }
      }
      case 0xDC: // Absolute call, conditional on C. 3 or 6 cycles,
                 // flags unmodified.
      {
        if (cpu.af.low & FLAG_Z) {
          uint16_t call_addr = (op+1).read_16();
          op_call(cpu, call_addr);
          return 6;
        } else {
          return 3;
        }
      }
      case 0xEC: // both illegal
      case 0xFC:
        illop(opcode);
        return 0;
      default:
        throw std::logic_error("Bad opcode bits");
      }
      break;
    case 0xd:
      switch (opcode) {
      case 0xCD: // Absolute call, unconditional. 6 cycles, flags
                 // unmodified.
      {
        uint16_t call_addr = (op+1).read_16();
        op_call(cpu, call_addr);
        return 6;
      }
      case 0xDD: // All 3 illegal
      case 0xED:
      case 0xFD:
        illop(opcode);
        return 0;
      default:
        throw std::logic_error("Bad opcode bits");
      }
      break;
    case 0xe: // More 8-bit math. All 2 cycles, modifies all flags.
      switch (opcode) {
      case 0xCE: // ADC A,d8
      {
        int result = cpu.af.high + (op+1).read() + !!(cpu.af.low & FLAG_C);
        // We carried from bit 3 iff bit 4 of the result isn't the same
        // as the XOR of bits 4 of the arguments
        int carryH = (result & (1<<4)) !=
          ((cpu.af.high & (1<<4)) ^ ((op+1).read() & (1<<4)));
        int carryC = !!(result & (1<<8));
        cpu.af.high = result & 0xff;
        cpu.updateFlags(!result, 0, carryH, carryC);
        return 2;
      }
      case 0xDE: // SBC A,d8
      {
        uint8_t subtractend = ~((op+1).read()) + 1;
        int result = cpu.af.high + subtractend - !!(cpu.af.low & FLAG_C);
        int carryH = (result & (1<<4)) !=
          ((cpu.af.high & (1<<4)) ^ (subtractend & (1<<4)));
        int carryC = !!(result & (1<<8));
        cpu.af.high = result & 0xff;
        cpu.updateFlags(!result, 1, carryH, carryC);
        return 2;
      }
      case 0xEE: // XOR d8
      {
        uint8_t result = cpu.af.high ^ (op+1).read();
        cpu.af.high = result;
        cpu.updateFlags(!result, 0, 0, 0);
        return 2;
      }
      case 0xFE: // CP d8
      {
        // Same as SUB, but don't change A
        uint8_t subtractend = ~((op+1).read()) + 1;
        int result = cpu.af.high + subtractend;
        int carryH = (result & (1<<4)) !=
          ((cpu.af.high & (1<<4)) ^ (subtractend & (1<<4)));
        int carryC = !!(result & (1<<8));
        cpu.updateFlags(!result, 1, carryH, carryC);
        return 2;
      }
      default:
        throw std::logic_error("Bad opcode bits");
      }
      break;
    case 0xf: // RST: address onto stack, jump to fixed address. 4
              // cycles, flags unmodified.
    {
      // AFAICT, this should push the address of the /next/
      // instruction. Documentation is unclear.
      uint16_t op_size = 3;
      uint16_t return_addr = cpu.pc + op_size;
      cpu.stack_push_16(return_addr);

      uint16_t call_addr = 8 + (opcode & 0x30);
      cpu.next_pc = call_addr;

      return 4;
    }
    default:
      throw std::logic_error("Bad opcode low bits");
    }
    throw std::logic_error("Exited switch statement"); // unreachable

  default:
    throw std::logic_error("Bad opcode high bits");
  }

  throw std::logic_error("Exited switch statement"); // unreachable
}

int cb_prefix_operate(CPU &cpu, uint8_t cb_op) {
  int did_increment = 0;
  gb_ptr arg = reg_8_all_or_indirect(cpu, (cb_op & 7), did_increment);
  switch (cb_op & 0xC0) {
  case 0x00:
    switch ((cb_op >> 3) & 7) {
    case 0: // RLC
    {
      unsigned int rotated = arg.read() << 1;
      arg.write((rotated & 0xff) + (rotated >> 8)); // rotate high bit to low bit
      // Looks like this behaves differently from RLCA, in that it
      // sets the Z flag according to the result.
      // TODO: confirm.
      cpu.updateFlags(!rotated, 0, 0, rotated >> 8);
      break;
    }
    case 1: // RRC
    {
      unsigned int rotated = arg.read() >> 1;
      int flagZ = !(arg.read());
      int flagC = (arg.read() & 1);
      arg.write(rotated + (flagC << 7));
      cpu.updateFlags(flagZ, 0, 0, flagC);
      break;
    }
    case 2: // RL
    {
      unsigned int rotated = arg.read() << 1;
      arg.write((rotated & 0xff) + !!(cpu.af.low & FLAG_C)); // rotate carry flag to low bit
      cpu.updateFlags(!rotated, 0, 0, rotated >> 8);
      break;
    }
    case 3: // RR
    {
      unsigned int rotated = arg.read() >> 1;
      int flagZ = !(arg.read());
      int flagC = (arg.read() & 1);
      arg.write(rotated + (!!(cpu.af.low & FLAG_C) << 7));
      cpu.updateFlags(flagZ, 0, 0, flagC);
      break;
    }
    case 4: // SLA (shift left into carry)
    {
      unsigned int rotated = arg.read() << 1;
      arg.write((rotated & 0xff)); // low bit is 0
      cpu.updateFlags(!(rotated & 0xff), 0, 0, rotated >> 8);
      break;
    }

    case 5: // SRA (shift right into carry, high bit stays same)
    {
      unsigned int rotated = arg.read() >> 1;
      int flagZ = !(arg.read());
      int flagC = (arg.read() & 1);
      arg.write(rotated + ((rotated << 1) & 0x80)); // high bit stays the same
      cpu.updateFlags(flagZ, 0, 0, flagC);
      break;
    }
    case 6: // SWAP (upper and lower nibbles)
    {
      uint8_t result = (arg.read() >> 4) + (arg.read() << 4);
      arg.write(result);
      cpu.updateFlags(!result, 0, 0, 0);
      break;
    }
    case 7: // SRL (shift right into carry, high bit cleared)
    {
      unsigned int rotated = arg.read() >> 1;
      int flagZ = !(arg.read());
      int flagC = (arg.read() & 1);
      arg.write(rotated);
      cpu.updateFlags(flagZ, 0, 0, flagC);
      break;
    }
    default:
      throw std::logic_error("Bad opcode bits");
    }
  case 0x40: // BIT
  {
    uint8_t bit = (cb_op >> 3) & 7;
    int flagZ = !(arg.read() & (1 << bit));
    cpu.updateFlags(flagZ, 0, 1, -1);
    break;
  }
  case 0x80: // RES
  {
    uint8_t bit = (cb_op >> 3) & 7;
    arg.write(arg.read() & ~((uint8_t) (1 << bit)));
    break;
  }
  case 0xC0: // SET
  {
    uint8_t bit = (cb_op >> 3) & 7;
    arg.write(arg.read() | (1 << bit));
    break;
  }
  default:
    throw std::logic_error("Bad opcode bits");
  }
  return 2 + (2*did_increment);
}
