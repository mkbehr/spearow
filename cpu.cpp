#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <iostream>
#include <limits>

#include "cpu.hpp"
#include "mem.hpp"
#include "opcodes.hpp"

CPU::CPU()
  : af({.full=0}), bc({.full=0}), de({.full=0}), hl({.full=0}),
    sp(INITIAL_SP), pc(INITIAL_PC), next_pc(0)
{
  memset(ram, 0, sizeof(ram));
  memset(highRam, 0, sizeof(highRam));

  // This sets up the CPU state to what it will be after the logo and
  // chime are displayed.

  // TODO: emulate the logo/chime program.
  postLogoSetup();
}

void CPU::postLogoSetup() {
  af.high = POSTLOGO_A;
  af.low = POSTLOGO_F;
  bc.full = POSTLOGO_BC;
  de.full = POSTLOGO_DE;
  hl.full = POSTLOGO_HL;
  sp = POSTLOGO_SP;
  pc = POSTLOGO_PC;
  // TODO: set up IO registers according to POSTLOGO_IOREG_INIT
}

void CPU::loadRom(const char *filepath) {
  std::ifstream romFile(filepath);
  if (!filepath) {
    std::cerr << "Couldn't open rom file\n";
    exit(0);
  }
  static_assert(sizeof(char) == sizeof(uint8_t),
                "char and uint8_t not the same size");
  // Quick way to cast the input chars into our vector of unsigned
  // chars: pretend the vector is signed chars
  std::vector<char> *r = (std::vector<char> *) &rom;
  r->assign(std::istreambuf_iterator<char>(romFile),
            std::istreambuf_iterator<char>());
  romFile.close();
}

void CPU::tick() {
  uint8_t op_first = gb_mem_ptr(*this, pc).read();
  next_pc = pc + OPCODE_LENGTHS[op_first];
  int cyclesElapsed = operate(*this, gb_mem_ptr(*this, pc));
  // The operation will change next_pc if necessary.
  pc = next_pc;
}

void CPU::updateFlags(int z, int n, int h, int c) {
  uint8_t flags = af.low;
  if (z == 0) {
    flags &= ~FLAG_Z;
  } else if (z > 0) {
    flags |= FLAG_Z;
  }
  if (n == 0) {
    flags &= ~FLAG_N;
  } else if (n > 0) {
    flags |= FLAG_N;
  }
  if (h == 0) {
    flags &= ~FLAG_H;
  } else if (h > 0) {
    flags |= FLAG_H;
  }
  if (h == 0) {
    flags &= ~FLAG_H;
  } else if (z > 0) {
    flags |= FLAG_H;
  }
  af.low = flags;
}

uint8_t CPU::stack_pop() {
  uint8_t out = gb_mem_ptr(*this, sp).read();
  sp++;
  return out;
}

void CPU::stack_push(uint8_t x) {
  sp--;
  gb_mem_ptr(*this, sp).write(x);
}


void CPU::stop() {
  fprintf(stderr, "Unimplemented feature: stop\n");
  exit(0);
}

void CPU::halt() {
  fprintf(stderr, "Unimplemented feature: halt\n");
  exit(0);
}

void CPU::enableInterrupts() {
  fprintf(stderr, "Warning: ignoring interrupt enable operation\n");
}

void CPU::disableInterrupts() {
  fprintf(stderr, "Warning: ignoring interrupt disable operation\n");
}

void CPU::printState() {
  printf("AF=%04x BC=%04x DE=%04x HL=%04x ",
         af.full, bc.full, de.full, hl.full);
  printf("SP=%04x PC=%04x ", sp, pc);
  printf("[%c%c%c%c]\n",
         (af.low & FLAG_Z) ? 'Z' : '-',
         (af.low & FLAG_N) ? 'N' : '-',
         (af.low & FLAG_H) ? 'H' : '-',
         (af.low & FLAG_C) ? 'C' : '-');
  uint8_t op_first = gb_mem_ptr(*this, pc).read();
  const char *opcode_name = op_first == 0xcb ?
    CB_OPCODE_NAMES[gb_mem_ptr(*this, pc+1).read()] :
    OPCODE_NAMES[op_first];
  printf("%04x: %s: %02x", pc, opcode_name, op_first);
  for (int i = 0; i < OPCODE_LENGTHS[op_first] - 1; i++) {
    uint8_t arg = gb_mem_ptr(*this, pc+i+1).read();
    printf(" %02x", arg);
  }
  printf("\n");
}
