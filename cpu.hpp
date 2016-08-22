#ifndef CPU_H

#define CPU_H

#include <cstdint>
#include <vector>

#include "screen.hpp"
#include "audio.hpp"

#ifndef __BYTE_ORDER__
#error Unknown byte order. Set __BYTE_ORDER__ to the appropriate value.
#endif

typedef union register_pair {
  struct {
    // Set struct ordering according to machine endianness.
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) || \
  (__BYTE_ORDER__ == __ORDER_PDP_ENDIAN__)
    unsigned char low;
    unsigned char high;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN
    unsigned char high;
    unsigned char low;
#else
#error Unrecognized byte order!
#endif
  };
  uint16_t full;
} register_pair;

const unsigned int CPU_CYCLES_PER_SECOND = 4194304;

// See screen.hpp for notes on this timing.
const unsigned int CPU_CYCLES_PER_FRAME = 70224;
const unsigned int CPU_CYCLES_PER_SCANLINE = 456;

const unsigned int RAM_SIZE = 0x2000;
const unsigned int OAM_SIZE = 0xa0;
const unsigned int HIGH_RAM_SIZE = 0x7f; // very top is IE register
const unsigned int VRAM_SIZE = 0x2000;

const int FLAG_Z = 1 << 7; // zero flag
const int FLAG_N = 1 << 6; // subtract flag
const int FLAG_H = 1 << 5; // half-carry flag
const int FLAG_C = 1 << 4; // carry flag

const uint8_t INT_VBLANK = 1<<0;
const uint16_t INT_VBLANK_ADDR = 0x0040;
const uint8_t INT_LCDC = 1<<1;
const uint16_t INT_LCDC_ADDR = 0x0048;
const uint8_t INT_TIMER = 1<<2;
const uint16_t INT_TIMER_ADDR = 0x0050;
const uint8_t INT_SERIAL = 1<<3;
const uint16_t INT_SERIAL_ADDR = 0x0058;
const uint8_t INT_JOYPAD = 1<<4;
const uint16_t INT_JOYPAD_ADDR = 0x0060;
const uint8_t INT_ALL = (INT_VBLANK |
                         INT_LCDC |
                         INT_TIMER |
                         INT_SERIAL |
                         INT_JOYPAD);

const int TIMER_PERIODS[4] = {
  CPU_CYCLES_PER_SECOND / 4096, // 1024
  CPU_CYCLES_PER_SECOND / 262144, // 16
  CPU_CYCLES_PER_SECOND / 65536, // 64
  CPU_CYCLES_PER_SECOND / 16384 // 256
};
const uint8_t TIMER_CONTROL_FREQ = 3;
const uint8_t TIMER_CONTROL_ENABLE = 1<<2;

const uint16_t CART_TYPE_ADDR = 0x0147;

const uint16_t INITIAL_SP = 0xfffe;
const uint16_t INITIAL_PC = 0x0000;

// TODO the value of A after the logo startup sequence should depend
// on gameboy type: gameboy and super gameboy are 0x01, gameboy pocket
// is 0xff, and gameboy color is 0x11.
const uint8_t POSTLOGO_A = 0x01;
const uint8_t POSTLOGO_F = 0xb0;
const uint16_t POSTLOGO_BC = 0x0013;
const uint16_t POSTLOGO_DE = 0x00d8;
const uint16_t POSTLOGO_HL = 0x014d;
const uint16_t POSTLOGO_SP = 0xfffe;
const uint16_t POSTLOGO_PC = 0x0100;

// Note: some zero values in here are explicitly initialized to zero;
// others are uninitialized in hardware. See the gameboy CPU manual,
// v. 1.01, page 18. $ffff is also explicitly initialized to zero.

// TODO: initial value of $ff26 depends on gameboy type. f1 for
// gameboy, f0 for super gameboy.
const uint8_t POSTLOGO_IOREG_INIT[128] =
{
  // ff00-ff0f
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  // ff10-ff1f
  0x80, 0xbf, 0xf3, 0x00, 0xbf, 0x00, 0x3f, 0x00,
  0x00, 0xbf, 0x7f, 0xff, 0x9f, 0x00, 0xbf, 0x00,
  // ff20-ff2f
  0xff, 0x00, 0x00, 0xbf, 0x77, 0xf3, 0xf1, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  // ff30-ff3f
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  // ff40-ff4f
  0x91, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc,
  0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  // ff50-ff5f
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  // ff60-ff6f
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  // ff70-ff7f
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

class Screen;
class Audio;

class CPU {
public:
  CPU(bool vsync = true, bool displayTiles = false);
  ~CPU();

  void printState();
  void printFlags(uint8_t);

  void tick();

  void loadRom(const char *);

  void updateFlags(int z, int n, int h, int c);

  register_pair af;
  register_pair bc;
  register_pair de;
  register_pair hl;
  uint16_t sp; // stack pointer
  uint16_t pc; // program counter

  uint16_t next_pc; // Next PC value, to be set by jumps

  uint8_t ram[RAM_SIZE];
  uint8_t highRam[HIGH_RAM_SIZE];
  uint8_t vram[VRAM_SIZE];
  uint8_t oam[OAM_SIZE];

  uint8_t cartridge_type;

  // MBC state.
  // TODO: find a better place to store MBC-specific state
  int rom_bank_low;
  int ram_bank; // or rom bank high
  int mbc_mode; // TODO make an enum

  // interrupt state
  uint8_t interrupts_raised;
  uint8_t interrupts_enabled;
  bool interrupt_master_enable;

  // timer/divider state
  uint16_t fine_divider;
  uint8_t timer_count;
  uint8_t timer_mod;
  uint8_t timer_control;

  // display registers
  uint8_t lcd_control;
  uint8_t lcd_status;
  uint8_t scroll_y;
  uint8_t scroll_x;
  uint8_t lcd_y;
  uint8_t lcd_y_compare;
  uint8_t bg_palette;
  uint8_t obj_palette_0;
  uint8_t obj_palette_1;
  uint8_t window_y;
  uint8_t window_x;

  uint8_t joypad_mask;

  // halt state
  bool halted;

  Screen *screen;
  Audio *audio;

  uint8_t stack_pop();
  void stack_push(uint8_t x);

  uint16_t stack_pop_16();
  void stack_push_16(uint16_t x);

  void stop(); // TODO
  void halt();

  void disableInterrupts();
  void enableInterrupts();

  void reset_lcd();

  std::vector<uint8_t> rom;

  // These have to be static to work with the signal handlers. This
  // will interact strangely if there are ever multiple CPUs.
  static bool debuggerRequested;
  static void handle_sigint(int);

  void install_sigint();
  static void uninstall_sigint();


private:
  void postLogoSetup();

  int cycles_to_next_frame;
  int cycles_to_next_scanline;

  void handleInterrupts();
  int load_op_and_execute();
  void timer_tick(int cyclesElapsed);
  void display_tick(int cyclesElapsed);

  // old SIGINT action handler
  static struct sigaction oldsigint;
};

#endif // #ifndef CPU_H
