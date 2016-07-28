#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <limits>

#include "cpu.hpp"

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

inline void CPU::updateFlags(int z, int n, int h, int c) {
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

uint8_t *CPU::mem_ptr(uint16_t addr) {
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
