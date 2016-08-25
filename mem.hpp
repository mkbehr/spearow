#ifndef MEM_H

#define MEM_H

#include "cpu.hpp"

const int MEM_WARN = 1;

const int MONITOR_LINK_PORT = 0;

const uint16_t RAM_BASE = 0xc000;
// COMPAT: the official manual says e000-fdff is forbidden. The
// unofficial manual says it's an echo of internal RAM.
const uint16_t RAM_ECHO_BASE = 0xe000;
const uint16_t RAM_ECHO_TOP = 0xfdff;

const uint16_t RAM_SWITCHABLE_BASE = 0xa000;
const uint16_t RAM_BANK_SIZE = 0x2000;

const uint16_t HIGH_RAM_BASE = 0xff80;

const uint16_t ROM_BASE = 0;
const uint16_t ROM_SWITCHABLE_BASE = 0x4000;
const uint16_t ROM_BANK_SIZE = 0x4000;

const uint16_t OAM_BASE = 0xfe00;
const uint16_t SPRITE_SIZE = 4;
const int OAM_N_SPRITES = OAM_SIZE / SPRITE_SIZE;

const uint16_t IO_BASE = 0xff00;
const uint16_t IO_SIZE = 0x80;

const uint16_t VRAM_BASE = 0x8000;

// Registers. Note: the GBC has more registers here. See the official
// programming manual.

// misc. registers
const uint16_t REG_JOYPAD = 0xff00; // R/W; also called P1
const uint16_t REG_SERIAL_DATA = 0xff01; // R/W; also called SB
const uint16_t REG_SERIAL_CONTROL = 0xff02; // R/W; also called SC
const uint16_t REG_DIVIDER = 0xff04; // R/W; DIV
const uint16_t REG_TIMER_COUNT = 0xff05; // R/W; TIMA
const uint16_t REG_TIMER_MOD = 0xff06; // R/W; TMA
const uint16_t REG_TIMER_CONTROL = 0xff07; // R/W; TAC
const uint16_t REG_INTERRUPT = 0xff0f; // R/W; IF
const uint16_t REG_INTERRUPT_ENABLE = 0xffff; // R/W; IE
// sound registers: R/W except NR 13, NR 23, NR 33
const uint16_t REG_SOUND_1_0 = 0xff10; // NR 10: sweep
const uint16_t REG_SOUND_1_1 = 0xff11; // NR 11: duty and duration
const uint16_t REG_SOUND_1_2 = 0xff12; // NR 12: envelope
const uint16_t REG_SOUND_1_3 = 0xff13; // NR 13: frequency low
const uint16_t REG_SOUND_1_4 = 0xff14; // NR 14: enable, duration enable, frequency high
const uint16_t REG_SOUND_2_1 = 0xff16; // NR 21: duty and duration
const uint16_t REG_SOUND_2_2 = 0xff17; // NR 22: envelope
const uint16_t REG_SOUND_2_3 = 0xff18; // NR 23: frequency low
const uint16_t REG_SOUND_2_4 = 0xff19; // NR 24: enable, duration enable, frequency high
const uint16_t REG_SOUND_3_0 = 0xff1a; // NR 30: enable
const uint16_t REG_SOUND_3_1 = 0xff1b; // NR 31: duration
const uint16_t REG_SOUND_3_2 = 0xff1c; // NR 32: envelope
const uint16_t REG_SOUND_3_3 = 0xff1d; // NR 33: frequency low
const uint16_t REG_SOUND_3_4 = 0xff1e; // NR 34: enable, duration enable, frequency high
const uint16_t REG_SOUND_4_BASE = 0xff20; // NR 41 through NR 44
const uint16_t REG_SOUND_4_SIZE = 4;
const uint16_t REG_SOUND_5_BASE = 0xff24; // NR 50 through NR 52
const uint16_t REG_SOUND_5_SIZE = 3;
const uint16_t REG_SOUND_5_0 = 0xff24; // NR 50: speaker volume and Vin
const uint16_t REG_SOUND_5_1 = 0xff25; // NR 51: channels to speakers
const uint16_t REG_SOUND_5_2 = 0xff26; // NR 52: channel enable
const uint16_t WAVE_RAM_BASE = 0xff30;
// display registers
const uint16_t REG_LCD_CONTROL = 0xff40; // R/W; LCDC
const uint16_t REG_LCD_STATUS = 0xff41; // R/W; STAT
const uint16_t REG_SCROLL_Y = 0xff42; // R/W; SCY
const uint16_t REG_SCROLL_X = 0xff43; // R/W; SCX
const uint16_t REG_LCD_Y = 0xff44; // R; LY
const uint16_t REG_LCD_Y_COMPARE = 0xff45; // R/W; LYC
const uint16_t REG_DMA = 0xff46; // W; DMA
const uint16_t REG_BG_PALETTE = 0xff47; // R/W; BGP
const uint16_t REG_OBJ_PALETTE_0 = 0xff48; // R/W; OBP0
const uint16_t REG_OBJ_PALETTE_1 = 0xff49; // R/W; OBP1
const uint16_t REG_WINDOW_Y = 0xff4a; // R/W; WY
const uint16_t REG_WINDOW_X = 0xff4b; // R/W; WX

enum mbc_type {
  MBC_NONE,
  MBC_1,
  MBC_2,
  MBC_3,
  MBC_4,
  MBC_5,
  MBC_HUC1,
  MBC_UNKNOWN
};

enum gb_ptr_type {
  GB_PTR_MEM,
  GB_PTR_REG,
  GB_PTR_TYPE_MAX
};

typedef union gb_ptr_val {
  uint16_t addr;
  uint8_t *reg;
} gb_ptr_val;

class gb_ptr {
public:
  gb_ptr(CPU &, const gb_ptr_type, const gb_ptr_val);
  uint8_t read();
  uint16_t read_16();
  void write(uint8_t);
  void write_16(uint16_t);
  gb_ptr operator+(int) const;
  gb_ptr operator-(int) const;
private:
  CPU &cpu;
  const gb_ptr_type ptr_type;
  const gb_ptr_val val;

};

typedef union gb_ptr_16_val {
  uint16_t addr;
  uint16_t *reg;
} gb_ptr_16_val;

class gb_ptr_16 {
public:
  gb_ptr_16(CPU &, const gb_ptr_type, const gb_ptr_16_val);
  uint16_t read();
  void write(uint16_t);
  gb_ptr_16 operator+(int) const;
  gb_ptr_16 operator-(int) const;
private:
  CPU &cpu;
  const gb_ptr_type ptr_type;
  const gb_ptr_16_val val;

};

gb_ptr gb_mem_ptr(CPU &, uint16_t);
gb_ptr gb_reg_ptr(CPU &, uint8_t *);
gb_ptr_16 gb_mem16_ptr(CPU &, uint16_t);
gb_ptr_16 gb_reg16_ptr(CPU &, uint16_t *);

mbc_type cart_mbc_type(CPU &);
int rom_bank_offset(CPU &);

#endif // #ifndef MEM_H
