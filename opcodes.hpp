#ifndef OPCODES_H

#define OPCODES_H

#include "mem.hpp"

const int TRACE_CALLS = 0;
const int COUNT_OPCODES = 1;

/*
  Summary of opcode table:
  0x00-0x3F: various.
    ?0: NOP; STOP 0; JR NZ,r8; JR NC,r8
    ?1: LD [BC, DE, HL, SP],d16
    ?2: LD ([BC, DE, HL+, HL-]),A
    ?3: INC [BC, DE, HL, SP]
    ?4: INC [B, D, H, (HL)]
    ?5: DEC [B, D, H, (HL)]
    ?6: LD [B, D, H, (HL)],d8
    ?7: RLCA; RLA; DAA; SCF
    ?8: LD (a16),SP; JR r8; JR Z,r8; JR C,r8
    ?9: ADD HL,[BC, DE, HL, SP]
    ?A: LD A,([BC, DE, HL+, HL-])
    ?B: DEC [BC, DE, HL, SP]
    ?C: INC [C, E, L, A]
    ?D: DEC [C, E, L, A]
    ?E: LD [C, E, L, A],d8
    ?F: RRCA; RRA; CPL; CCF

  0x40-0x7F: 8-bit LD operations. Single exception: 0x76 is HALT.
    First argument:
    0x40-0x47: B
    0x48-0x4F: C
    0x50-0x57: D
    0x58-0x5F: E
    0x60-0x67: H
    0x68-0x6F: L
    0x70-0x77: (HL)
    0x78-0x7F: A

    Second argument:
    ?0, ?8: B
    ?1, ?9: C
    ?2, ?A: D
    ?3, ?B: E
    ?4, ?C: H
    ?5, ?D: L
    ?6, ?E: (HL)
    ?7, ?F: A


  0x80-0xBF: 8-bit math/logic operations.
    0x80-0x87: ADD A,
    0x88-0x8F: ADC A,
    0x90-0x97: SUB
    0x98-0x9F: SBC A,
    0xA0-0xA7: AND
    0xA8-0xAF: XOR
    0xB0-0xB7: OR
    0xB8-0xBF: CP

    (Second) argument: same as 0x40-0x7f

  0xC0-0xFF: various.
    ?0: RET NZ; RET NC; LDH (a8),A; LDH A,(a8)
    ?1: POP [BC, DE, HL, AF]
    ?2: JP NZ,a16; JP NC,a16; LD (C),A; LD A,(C)
    ?3: JP a16; ---; ---; DI
    ?4: CALL NZ,a16; CALL NC,a16; ---; ---
    ?5: PUSH [BC, DE, HL, AF]
    ?6: [ADD A, SUB, AND, OR] d8
    ?7: RST [00H, 10H, 20H, 30H]
    ?8: RET Z; RET C; ADD SP,r8; LD HL,SP+r8
    ?9: RET; RETI; JP (HL); LD SP,HL
    ?A: JP Z,a16; JP C,a16; LD (a16),A; LD A,(a16)
    ?B: Secondary opcode table CB??; ---; ---; EI
    ?C: CALL Z,a16; CALL C,a16; ---; ---
    ?D: CALL a16; ---; ---; ---
    ?E: [ADC A, SBC A, XOR, CP] d8
    ?F: RST [08H, 18H, 28H, 38H]


  Secondary opcode table (all 8-bit rotations/bitshifts):

  0xCB00-0xCB07: RLC
  0xCB08-0xCB0F: RRC

  0xCB10-0xCB17: RL
  0xCB18-0xCB1F: RR

  0xCB20-0xCB27: SLA
  0xCB28-0xCB2F: SRA

  0xCB30-0xCB37: SWAP
  0xCB38-0xCB3F: SRL


  0xCB40-0xCB7F: BIT

  0xCB80-0xCBBF: RES

  0xCBC0-0xCBCF: SET

  First argument of BIT, RES and SET:
  ((secondary opcode) >> 3) & 7

  (Second) argument: same as 0x40-0x7f
 */

const unsigned char OPC_HALT = 0x76;

int operate(CPU &cpu, gb_ptr op);
int cb_prefix_operate(CPU &cpu, uint8_t cb_op);

extern const int OPCODE_LENGTHS[256];

extern const char *OPCODE_NAMES[256];

extern const char *CB_OPCODE_NAMES[256];

extern long opcode_counts[256];

#endif
