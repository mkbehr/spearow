#include <cassert>
#include <csignal>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <iostream>
#include <limits>

#include "cpu.hpp"
#include "debugger.hpp"
#include "mem.hpp"
#include "opcodes.hpp"

CPU::CPU(bool vsync, bool displayTiles)
  : af({.full=0}), bc({.full=0}), de({.full=0}), hl({.full=0}),
    sp(INITIAL_SP), pc(INITIAL_PC), next_pc(0),
    interrupts_raised(0),
    interrupts_enabled(0), interrupt_master_enable(0),
    fine_divider(0),
    timer_count(0), timer_mod(0), timer_control(0),
    halted(0),
    rom_bank_low(1), ram_bank(0), mbc_mode(0),
    cycles_to_next_frame(CPU_CYCLES_PER_FRAME),
    cycles_to_next_scanline(CPU_CYCLES_PER_SCANLINE),
    screen(new Screen(this, vsync, displayTiles)),
    audio(new Audio(this))
{
  install_sigint();

  memset(ram, 0, sizeof(ram));
  memset(highRam, 0, sizeof(highRam));

  audio->apuInit();

  // This sets up the CPU state to what it will be after the logo and
  // chime are displayed.

  // TODO: emulate the logo/chime program.
  postLogoSetup();
}

CPU::~CPU() {
  // restore the SIGINT handler to its old behavior
  uninstall_sigint();
}

bool CPU::debuggerRequested;
struct sigaction CPU::oldsigint;

void CPU::install_sigint() {
  const struct sigaction act =
    {.sa_handler = this->handle_sigint,
     .sa_mask = 0,
     .sa_flags = 0};
  sigaction(SIGINT, &act, &oldsigint);
  assert(oldsigint.sa_handler != this->handle_sigint);
}

void CPU::uninstall_sigint() {
  struct sigaction cpu_sigint;
  sigaction(SIGINT, &oldsigint, &cpu_sigint);
}

void CPU::handle_sigint(int signum) {
  debuggerRequested = 1;
  uninstall_sigint();
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

  cartridge_type = rom.at(CART_TYPE_ADDR);
}

void CPU::handleInterrupts() {
  // handle interrupts
  if (interrupt_master_enable || halted) {
    uint8_t interrupts = interrupts_enabled & interrupts_raised & INT_ALL;
    if (interrupts) {
      // However, if we are halted but the interrupt master enable
      // flag is unset, then we're not going to actually interrupt.
      if (halted && !interrupt_master_enable) {
        // COMPAT: Unclear: in this case, do we clear the
        // corresponding bit in the interrupts_raised register or not?
        // I guess the test suite suggests that we shouldn't?

        // COMPAT: On anything but a gameboy color, this case should
        // cause us to skip advancing the program counter on the next
        // instruction fetch.
        halted = 0;
      } else {
        // unhalt if we were halted
        halted = 0;

        // now do the interrupt
        uint8_t chosen_interrupt;
        uint16_t addr;
        if (interrupts & INT_VBLANK) {
          chosen_interrupt = INT_VBLANK;
          addr = INT_VBLANK_ADDR;
        } else if (interrupts & INT_LCDC) {
          chosen_interrupt = INT_LCDC;
          addr = INT_LCDC_ADDR;
        } else if (interrupts & INT_TIMER) {
          chosen_interrupt = INT_TIMER;
          addr = INT_TIMER_ADDR;
        } else if (interrupts & INT_SERIAL) {
          chosen_interrupt = INT_SERIAL;
          addr = INT_SERIAL_ADDR;
        } else {
          assert(interrupts & INT_JOYPAD);
          chosen_interrupt = INT_JOYPAD;
          addr = INT_JOYPAD_ADDR;
        }
        // Interrupt procedure:
        // - unset corresponding bit in request register
        // - unset master enable flag
        // - push PC onto stack
        // - jump to interrupt vector
        interrupts_raised &= ~chosen_interrupt;
        interrupt_master_enable = 0;
        stack_push_16(pc);
        pc = addr;
      }
    }
  }
}

int CPU::load_op_and_execute() {
  int cyclesElapsed;
  uint8_t op_first = gb_mem_ptr(*this, pc).read();
  next_pc = pc + OPCODE_LENGTHS[op_first];
  cyclesElapsed = operate(*this, gb_mem_ptr(*this, pc));
  // The operation will change next_pc if necessary.
  pc = next_pc;
  return cyclesElapsed;
}

void CPU::timer_tick(int cyclesElapsed) {
  // Process timer and divider. See
  // http://gbdev.gg8.se/wiki/articles/Timer_Obscure_Behaviour for
  // details. Also triggers audio frame tick.
  int clockCyclesElapsed = cyclesElapsed * 4;
  uint16_t newDivider = fine_divider + clockCyclesElapsed;
  // TODO: double-speed cpu checks bit 14 instead of bit 13
  if ((fine_divider & (1<<13)) &&
      ~(newDivider & (1<<13))) {
    // TODO sound clock
  }
  if (timer_control & TIMER_CONTROL_ENABLE) {
    // Timer is driven by a falling edge detector on one bit of
    // fine_divider. Which bit that is depends on the lower two bits of
    // the timer control.
    int timer_bit = 3 + ((timer_control & TIMER_CONTROL_FREQ) * 2);
    if ((fine_divider & (1<<timer_bit)) &&
        ~(newDivider & (1<<timer_bit))) {
      // Tick the timer.

      // COMPAT this behavior isn't exact. The interrupt is actually set
      // four clock-cycles later, and there's some unexpected behavior
      // with certain write timings.

      timer_count++;
      if (!timer_count) {
        timer_count = timer_mod;
        interrupts_raised |= INT_TIMER;
      }
    }
  }
  fine_divider = newDivider;
}

void CPU::audio_frame_tick() {
  audio->frameTick();
}

void CPU::display_tick(int cyclesElapsed) {
  int clockCyclesElapsed = cyclesElapsed * 4;
  // Also process frame timing. This will have to be much more
  // sophisticated eventually, but this'll work for now.
  cycles_to_next_scanline -= clockCyclesElapsed;
  if (cycles_to_next_scanline <= 0) {
    lcd_y = (lcd_y + 1) % (SCREEN_HEIGHT + VBLANK_HEIGHT);
    cycles_to_next_scanline += CPU_CYCLES_PER_SCANLINE;
    // TODO: generate INT_LCDC according to the lcd status register
    if (lcd_y >= SCREEN_HEIGHT) {
      // we have started vblank; request the vblank interrupt
      interrupts_raised |= INT_VBLANK;
    }
    if (lcd_y == 0) {
      // we're out of vblank; unrequest vblank interrupt
      // FIXME: this might not actually be real
      interrupts_raised &= ~INT_VBLANK;
    }
  }
  // TODO compare lcd_y with lcd_y_compare
  cycles_to_next_frame -= clockCyclesElapsed;
  if (cycles_to_next_frame <= 0) {
    screen->draw();
    cycles_to_next_frame += CPU_CYCLES_PER_FRAME;
  }
}

void CPU::tick() {
  handleInterrupts();
  int cyclesElapsed;
  if (!halted) {
    cyclesElapsed = load_op_and_execute();
  } else {
    cyclesElapsed = 1;
  }

  timer_tick(cyclesElapsed);

  if (lcd_control & 0x80) {
    display_tick(cyclesElapsed);
  }

  // Now break into the debugger, if requested

  // FIXME: this'll currently let us recursively enter the debugger,
  // which is kind of weird. fix that logic.
  if (debuggerRequested) {
    debuggerRequested = 0;
    printf("\n");
    run_debugger(*this);
    // once we're out of the debugger, we can reinstall our SIGINT
    // handler.
    install_sigint();
  }
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
  if (c == 0) {
    flags &= ~FLAG_C;
  } else if (c > 0) {
    flags |= FLAG_C;
  }
  af.low = flags;
}

// I implemented 8-bit pushes and pops, but they aren't actually used!
// All PUSH and POP operations are 16-bit, and everything else that
// interacts with the stack works in terms of 16-bit addresses.

uint8_t CPU::stack_pop() {
  uint8_t out = gb_mem_ptr(*this, sp).read();
  sp++;
  return out;
}

void CPU::stack_push(uint8_t x) {
  sp--;
  gb_mem_ptr(*this, sp).write(x);
}

/*
  Intel 8080 programmer's manual has this to say about byte ordering
  on the stack:

  """
  (1) The most significant 8 bits of data are stored at the memory
  address one less than the contents of the stack pointer.

  (2) The least significant 8 bits of data are stored at the memory
  address two less than the contents of the stack pointer.

  (3) The stack pointer is automatically decremented by two.
  """

  The gameboy's processor is related to the 8080, so I'm going to
  assume the stack works the same way. It's also the more-convenient
  way.
 */

uint16_t CPU::stack_pop_16() {
  uint16_t out = gb_mem16_ptr(*this, sp).read();
  sp += 2;
  return out;
}

void CPU::stack_push_16(uint16_t x) {
  sp -=2;
  gb_mem16_ptr(*this, sp).write(x);
}


void CPU::stop() {
  // fprintf(stderr, "Unimplemented feature: stop\n");
  // exit(0);
}

void CPU::halt() {
  halted = 1;
}

void CPU::enableInterrupts() {
  // TODO: this actually shouldn't enable interrupts until the next
  // instruction, at least in the case of EI
  interrupt_master_enable = 1;
}

void CPU::disableInterrupts() {
  // TODO: this actually shouldn't disable interrupts until the next
  // instruction, at least in the case of DI
  interrupt_master_enable = 0;
}

void CPU::reset_lcd() {
  // Call when 0 is written to bit 7 of REG_LCD_CONTROL.
  lcd_y = 0;
  cycles_to_next_frame = CPU_CYCLES_PER_FRAME;
  cycles_to_next_scanline = CPU_CYCLES_PER_SCANLINE;
}

void CPU::printState() {
  printf("AF=%04x BC=%04x DE=%04x HL=%04x ",
         af.full, bc.full, de.full, hl.full);
  printf("SP=%04x PC=%04x ", sp, pc);
  printFlags(af.low);
  printf("\n");
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

void CPU::printFlags(uint8_t flags) {
  printf("[%c%c%c%c]",
         (flags & FLAG_Z) ? 'Z' : '-',
         (flags & FLAG_N) ? 'N' : '-',
         (flags & FLAG_H) ? 'H' : '-',
         (flags & FLAG_C) ? 'C' : '-');
}
