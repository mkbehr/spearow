#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <iostream>
#include <limits>

#include "cpu.hpp"
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
  uint8_t op_first = *mem_ptr(pc);
  next_pc = pc + OPCODE_LENGTHS[op_first];
  // Note: passing a pointer here is iffy, because we might cross the
  // memory bank boundary. Replacing mem_ptr with read/write functions
  // will fix that.
  int cyclesElapsed = operate(*this, mem_ptr(pc));
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

// TODO: this should really be implemented as read/write functions
// instead of just a pointer
uint8_t *CPU::mem_ptr(uint16_t addr) {

  // TODO: respect switchable ROM bank
  if ((ROM_BASE <= addr) &&
      (addr < ROM_SWITCHABLE_BASE + ROM_BANK_SIZE)) {

    return &rom.at(addr - ROM_BASE);
  }

  // TODO: respect switchable RAM bank
  if ((RAM_BASE <= addr) &&
      (addr < RAM_BASE + RAM_SIZE)) {
    return &(ram[addr - RAM_BASE]);
  }

  if ((RAM_ECHO_BASE <= addr) &&
      (addr <= RAM_ECHO_TOP)) {
    return &(ram[addr - RAM_ECHO_BASE]);
  }

  if (HIGH_RAM_BASE <= addr) {
    // Test addr against edge of high ram with a static assert to
    // avoid -Wtautological-constant-out-of-range-compare complaining
    static_assert(
      std::numeric_limits< decltype(addr) >::max()
      < (int) HIGH_RAM_BASE + (int) HIGH_RAM_SIZE,
      "High ram should occupy the top of memory");
    return &(highRam[addr - HIGH_RAM_BASE]);
  }

  fprintf(stderr, "Access to unimplemented address %04x\n", addr);
  exit(0);
}

uint8_t CPU::stack_pop() {
  uint8_t out = *mem_ptr(sp);
  sp++;
  return out;
}

void CPU::stack_push(uint8_t x) {
  sp--;
  *mem_ptr(sp) = x;
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
  uint8_t op_first = *mem_ptr(pc);
  printf("%04x: %s: %02x", pc, OPCODE_NAMES[op_first], op_first);
  for (int i = 0; i < OPCODE_LENGTHS[op_first] - 1; i++) {
    uint8_t arg = *mem_ptr(pc+i+1);
    printf(" %02x", arg);
  }
  printf("\n");
}
