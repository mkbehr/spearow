#include <cassert>
#include <cstdio>
#include <cstdlib>

#include "mem.hpp"

// TODO cache this or make it const or something
mbc_type cart_mbc_type(CPU &cpu) {
  switch (cpu.cartridge_type) {
  case 0:
    return MBC_NONE;
  case 1:
  case 2:
  case 3:
    return MBC_1;
  case 5:
  case 6:
    return MBC_2;
  case 8:
  case 9:
    return MBC_NONE;
  case 0xb:
  case 0xc:
  case 0xd:
    // MMM01
    return MBC_UNKNOWN;
  case 0xf:
  case 0x10:
  case 0x11:
  case 0x12:
  case 0x13:
    return MBC_3;
  case 0x15:
  case 0x16:
  case 0x17:
    return MBC_4;
  case 0x19:
  case 0x1a:
  case 0x1b:
  case 0x1c:
  case 0x1d:
  case 0x1e:
    return MBC_5;
  case 0xfc: // pocket camera
  case 0xfd: // bandai tama5
  case 0xfe: // HuC3
    return MBC_UNKNOWN;
  case 0xff:
    return MBC_HUC1;
  default:
    return MBC_UNKNOWN;
  }
}

int rom_bank_offset(CPU &cpu) {
  switch (cart_mbc_type(cpu)) {
  case MBC_NONE:
    return 1 * ROM_BANK_SIZE;
  case MBC_1:
  {
    int rbl = cpu.rom_bank_low;
    if (rbl == 0) {
      rbl = 1;
    }
    if (cpu.mbc_mode == 0) {
      // ROM select mode
      return (rbl + ((cpu.ram_bank & 0x3) << 5)) * ROM_BANK_SIZE;
    } else {
      // RAM select mode
      return rbl * ROM_BANK_SIZE;
    }
  }
  case MBC_2:
  {
    // Only 16 ROM banks
    // COMPAT: confirm what happens when an invalid bank number is selected
    int bank = cpu.rom_bank_low & 0xf;
    if (bank == 0) {
      bank = 1;
    }
    return bank * ROM_BANK_SIZE;
  }
  case MBC_3:
  {
    // We just have one byte for selecting the rom bank
    int bank = cpu.rom_bank_low;
    if (bank == 0) {
      bank = 1;
    }
    return bank * ROM_BANK_SIZE;
  }
  default:
    fprintf(stderr, "Unimplemented MBC %d: Can't interpret ROM bank offset\n",
            cart_mbc_type(cpu));
    exit(-1);
    // TODO
  }
}

int ram_bank_offset(CPU &cpu) {
  switch (cart_mbc_type(cpu)) {
  case MBC_NONE:
    return 0;
  case MBC_1:
  {
    if (cpu.mbc_mode == 0) {
      // ROM select mode
      return 0;
    } else {
      // RAM select mode
      return cpu.ram_bank * RAM_BANK_SIZE;
    }
  }
  case MBC_2:
    return 0;
  case MBC_3:
  {
    return cpu.ram_bank * RAM_BANK_SIZE;
  }
  default:
    fprintf(stderr, "Unimplemented MBC %d: Can't interpret RAM bank offset\n",
            cart_mbc_type(cpu));
    exit(-1);
    // TODO other MBCs
  }
}

// BEGIN GB_PTR

gb_ptr::gb_ptr(CPU &c, const gb_ptr_type t, const gb_ptr_val v)
  : cpu(c), ptr_type(t), val(v)
{
  assert(ptr_type < GB_PTR_TYPE_MAX);
}

uint8_t gb_ptr::read() {

  switch(ptr_type) {

  case GB_PTR_MEM:
  {
    const uint16_t addr = val.addr;

    // 0x0000
    if ((ROM_BASE <= addr) &&
        (addr < ROM_BASE + ROM_BANK_SIZE)) {

      return cpu.rom.at(addr - ROM_BASE);
    }

    // 0x4000
    if ((ROM_SWITCHABLE_BASE <= addr) &&
        (addr < ROM_SWITCHABLE_BASE + ROM_BANK_SIZE)) {
      return cpu.rom.at(addr - ROM_SWITCHABLE_BASE + rom_bank_offset(cpu));
    }

    // 0x8000
    if ((VRAM_BASE <= addr) &&
        (addr < VRAM_BASE + VRAM_SIZE)) {
      return cpu.vram[addr - VRAM_BASE];
    }

    // 0xa000
    if ((RAM_SWITCHABLE_BASE <= addr) &&
        (addr < RAM_SWITCHABLE_BASE + RAM_BANK_SIZE)) {
      switch (cart_mbc_type(cpu)) {
      case MBC_3:
        if (cpu.ram_bank > 0x07) {
          // This is actually the timer, not RAM
          return 0; // TODO timer
        }
        break;
      default:
        // TODO make sure nothing other than MBC3 cares about this
        break;
      }
      if (cpu.expansionRamEnabled) {
          return cpu.expansionRam[addr - RAM_SWITCHABLE_BASE + ram_bank_offset(cpu)];
        } else {
          return 0;
        }
    }

    // 0xc000
    if ((RAM_BASE <= addr) &&
        (addr < RAM_BASE + RAM_SIZE)) {
      return cpu.ram[addr - RAM_BASE];
    }

    // 0xe000
    if ((RAM_ECHO_BASE <= addr) &&
        (addr <= RAM_ECHO_TOP)) {
      return cpu.ram[addr - RAM_ECHO_BASE];
    }

    // 0xff30
    if ((WAVE_RAM_BASE <= addr) &&
        (addr < WAVE_RAM_BASE + WAVE_RAM_SIZE)) {
      return cpu.waveRam[addr - WAVE_RAM_BASE];
    }

    // 0xff00
    // COMPAT: the official gameboy programming manual suggests that
    // the unused addresses here might just be regular RAM? Unclear.
    if ((IO_BASE <= addr) &&
        (addr < IO_BASE + IO_SIZE)) {
      switch (addr) {
        // misc
      case REG_JOYPAD:
        return cpu.screen->getKeys(cpu.joypad_mask);
        // Pressed buttons are 0, unpressed are 1. For now, don't
        // claim to be pressing all buttons at all times.
        return 0x3f;
      case REG_SERIAL_DATA:
        break;
      case REG_SERIAL_CONTROL:
        break;
      case REG_DIVIDER:
        return cpu.fine_divider >> 8;
        break;
      case REG_TIMER_COUNT:
        return cpu.timer_count;
      case REG_TIMER_MOD:
        return cpu.timer_mod;
      case REG_TIMER_CONTROL:
        return cpu.timer_control;
      case REG_INTERRUPT:
        return cpu.interrupts_raised;
      // sound
      // COMPAT: unused bits should be set to 1, not 0
      case REG_SOUND_1_0:
        return cpu.audio->pulses.at(0).read_sweep_control() & 0x7f;
      case REG_SOUND_1_1:
        // can read duty, but not duration control
        return cpu.audio->pulses.at(0).read_duty_control() << 6;
      case REG_SOUND_1_2:
        return cpu.audio->pulses.at(0).read_envelope_control();
      case REG_SOUND_1_3:
        // write-only
        return 0;
      case REG_SOUND_1_4:
        // can only read duration-enable bit
        return cpu.audio->pulses.at(1).read_duration_enable() ? (1<<6) : 0;
      case REG_SOUND_2_1:
        // can read duty, but not duration control
        return cpu.audio->pulses.at(1).read_duty_control() << 6;
      case REG_SOUND_2_2:
        return cpu.audio->pulses.at(1).read_envelope_control();
      case REG_SOUND_2_3:
        // write-only
        return 0;
      case REG_SOUND_2_4:
        // can only read duration-enable bit
        return cpu.audio->pulses.at(1).read_duration_enable() ? (1<<6) : 0;
      case REG_SOUND_3_0:
        return cpu.audio->custom.read_enabled() ? 0x80 : 0;
      case REG_SOUND_3_1:
        // write-only
        return 0;
      case REG_SOUND_3_2:
        // wave unit handles bit-shifting
        return cpu.audio->custom.read_envelope_control();
      case REG_SOUND_3_3:
        // write-only
        return 0;
      case REG_SOUND_3_4:
        return cpu.audio->custom.read_duration_enable() ? (1<<6) : 0;
      case REG_SOUND_5_0:
        return cpu.audio_volume;
      case REG_SOUND_5_1:
        return cpu.audio_terminals;
      // TODO other sound
      // display
      case REG_LCD_CONTROL:
        return cpu.lcd_control;
      case REG_LCD_STATUS:
        // TODO compute status
        return cpu.lcd_status;
      case REG_SCROLL_Y:
        return cpu.scroll_y;
      case REG_SCROLL_X:
        return cpu.scroll_x;
      case REG_LCD_Y:
        return cpu.lcd_y;
      case REG_LCD_Y_COMPARE:
        return cpu.lcd_y_compare;
      case REG_DMA: // write-only
        return 0;
      case REG_BG_PALETTE:
        return cpu.bg_palette;
      case REG_OBJ_PALETTE_0:
        return cpu.obj_palette_0;
      case REG_OBJ_PALETTE_1:
        return cpu.obj_palette_1;
      case REG_WINDOW_Y:
        return cpu.window_y;
      case REG_WINDOW_X:
        return cpu.window_x;
      default:
        break;
      }
    }

    // 0xfe00
    if ((OAM_BASE <= addr) &&
        (addr < OAM_BASE + OAM_SIZE)) {
      return cpu.oam[addr - OAM_BASE];
    }

    // 0xff80
    if ((HIGH_RAM_BASE <= addr) &&
        (addr < HIGH_RAM_BASE + HIGH_RAM_SIZE)) {
      return cpu.highRam[addr - HIGH_RAM_BASE];
    }

    // 0xffff
    if (addr == REG_INTERRUPT_ENABLE) {
      return cpu.interrupts_enabled;
    }

    if (MEM_WARN) {
      fprintf(stderr, "Reading 0 from unimplemented address %04x\n", addr);
    }
    return 0;
  }
  case GB_PTR_REG:
  {
    return *(val.reg);
  }
  default:
    fprintf(stderr, "gb_ptr::read(): bad ptr_type %d\n", ptr_type);
    exit(0);
  }
}

uint16_t gb_ptr::read_16() {
  assert(ptr_type == GB_PTR_MEM);
  return gb_mem16_ptr(cpu, val.addr).read();
}

void gb_ptr::write(uint8_t to_write) {

  switch(ptr_type) {

  case GB_PTR_MEM:
  {
    const uint16_t addr = val.addr;

    // 0x8000
    if ((VRAM_BASE <= addr) &&
        (addr < VRAM_BASE + VRAM_SIZE)) {
      cpu.vram[addr - VRAM_BASE] = to_write;
      return;
    }

    // 0xa000
    if ((RAM_SWITCHABLE_BASE <= addr) &&
        (addr < RAM_SWITCHABLE_BASE + RAM_BANK_SIZE)) {
      switch (cart_mbc_type(cpu)) {
      case MBC_3:
        if (cpu.ram_bank > 0x07) {
          // This is actually the timer, not RAM
          return; // TODO timer
        }
        break;
      default:
        // TODO make sure nothing other than MBC3 cares about this
        break;
      }
      if (cpu.expansionRamEnabled) {
        cpu.expansionRam[addr - RAM_SWITCHABLE_BASE + ram_bank_offset(cpu)] = to_write;
      }
      return;
    }

    // 0xc000
    if ((RAM_BASE <= addr) &&
        (addr < RAM_BASE + RAM_SIZE)) {
      cpu.ram[addr - RAM_BASE] = to_write;
      return;
    }

    // 0xe000
    if ((RAM_ECHO_BASE <= addr) &&
        (addr <= RAM_ECHO_TOP)) {
      cpu.ram[addr - RAM_ECHO_BASE] = to_write;
      return;
    }

    // 0xff30
    if ((WAVE_RAM_BASE <= addr) &&
        (addr < WAVE_RAM_BASE + WAVE_RAM_SIZE)) {
      cpu.waveRam[addr - WAVE_RAM_BASE] = to_write;
      return;
    }

    // 0xff00
    if ((IO_BASE <= addr) &&
        (addr < IO_BASE + IO_SIZE)) {
      switch (addr) {
      case REG_JOYPAD:
        cpu.joypad_mask = to_write & (JOYPAD_DIRECTIONS | JOYPAD_BUTTONS);
        return;
      case REG_SERIAL_DATA:
        if (MONITOR_LINK_PORT) {
          printf("%c", to_write);
          fflush(stdout);
        }
        // TODO
        return;
      case REG_SERIAL_CONTROL:
        // TODO
        return;
      case REG_DIVIDER:
        cpu.fine_divider = 0; // ignore given value
        return;
      case REG_TIMER_COUNT:
        cpu.timer_count = to_write;
        return;
      case REG_TIMER_MOD:
        cpu.timer_mod = to_write;
        return;
      case REG_TIMER_CONTROL:
        cpu.timer_control = to_write;
        return;
      case REG_INTERRUPT:
        // COMPAT Are these masks correct? Unclear.
        cpu.interrupts_raised = to_write & INT_ALL;
        return;
      // sound
      case REG_SOUND_1_0:
        cpu.audio->pulses.at(0).write_sweep_control(to_write);
        return;
      case REG_SOUND_1_1:
        cpu.audio->pulses.at(0).write_duration_control(to_write & 0x3f);
        cpu.audio->pulses.at(0).write_duty_control(to_write >> 6);
        return;
      case REG_SOUND_1_2:
        cpu.audio->pulses.at(0).write_envelope_control(to_write);
        return;
      case REG_SOUND_1_3:
        // COMPAT: what happens when you write to one of these
        // registers but not the other, after the sweep unit has
        // changed the frequency?
        cpu.audio->pulses.at(0).write_frequency_low(to_write);
        return;
      case REG_SOUND_1_4:
        // high frequency bits
        cpu.audio->pulses.at(0).write_frequency_high(to_write & 0x7);
        // duration-enable bit
        cpu.audio->pulses.at(0).write_duration_enable(!!(to_write & (1<<6)));
        // reset bit
        if (to_write & (1<<7)) {
          cpu.audio->pulses.at(0).reset();
        }
        return;
      case REG_SOUND_2_1:
        cpu.audio->pulses.at(1).write_duration_control(to_write & 0x3f);
        cpu.audio->pulses.at(1).write_duty_control(to_write >> 6);
        return;
      case REG_SOUND_2_2:
        cpu.audio->pulses.at(1).write_envelope_control(to_write);
        return;
      case REG_SOUND_2_3:
        // COMPAT: what happens when you write to one of these
        // registers but not the other, after the sweep unit has
        // changed the frequency?
        cpu.audio->pulses.at(1).write_frequency_low(to_write);
        return;
      case REG_SOUND_2_4:
        // high frequency bits
        cpu.audio->pulses.at(1).write_frequency_high(to_write & 0x7);
        // duration-enable bit
        cpu.audio->pulses.at(1).write_duration_enable(!!(to_write & (1<<6)));
        // reset bit
        if (to_write & (1<<7)) {
          cpu.audio->pulses.at(1).reset();
        }
        return;
      case REG_SOUND_3_0:
        cpu.audio->custom.write_enabled(to_write & 0x80);
        return;
      case REG_SOUND_3_1:
        cpu.audio->custom.write_duration(to_write);
        return;
      case REG_SOUND_3_2:
        // unit implementation handles bit shifting
        cpu.audio->custom.write_envelope_control(to_write);
        return;
      case REG_SOUND_3_3:
        cpu.audio->custom.write_frequency_low(to_write);
        return;
      case REG_SOUND_3_4:
        // high frequency bits
        cpu.audio->custom.write_frequency_high(to_write & 0x7);
        // duration-enable bit
        cpu.audio->custom.write_duration_enable(!!(to_write & (1<<6)));
        // reset bit
        if (to_write & (1<<7)) {
          // load samples
          std::vector<uint8_t> inSamples;
          for (unsigned int i = 0; i < WAVE_RAM_SIZE; i++) {
            inSamples.push_back(gb_mem_ptr(cpu, WAVE_RAM_BASE+i).read());
          }
          cpu.audio->custom.reset(inSamples);
        }
        return;
      case REG_SOUND_5_0:
        cpu.audio_volume = to_write;
        return;
      case REG_SOUND_5_1:
        cpu.audio_terminals = to_write;
        return;
      // TODO other sound
      // Video registers
      // COMPAT Are any of these masked?
      case REG_LCD_CONTROL:
        cpu.lcd_control = to_write;
        if (!(cpu.lcd_control & 0x80)) {
          cpu.reset_lcd();
        }
        return;
      case REG_LCD_STATUS:
        // TODO update interrupts
        cpu.lcd_status = to_write;
        return;
      case REG_SCROLL_Y:
        cpu.scroll_y = to_write;
        return;
      case REG_SCROLL_X:
        cpu.scroll_x = to_write;
        return;
      case REG_LCD_Y: // read-only
        return;
      case REG_LCD_Y_COMPARE:
        cpu.lcd_y_compare = to_write;
        return;
      case REG_DMA:
      {
        // COMPAT: in the original game boy, transfer address must be
        // between 0x8000 and 0xdfff (what happens otherwise?)

        // COMPAT: handle behavior when OAM is unavailable (the next
        // 160 microseconds, or somewhere around 671 cpu clock cycles?
        // official programming manual seems to say it's actually
        // 160*4=640 cpu clock cycles.)

        // COMPAT: this transfer should happen over time

        // COMPAT: during the transfer, all memory except high RAM
        // should be unavailable

        // COMPAT: it's possible that the lower nibble of the flag
        // byte isn't actually written here? unclear.

        uint16_t dma_addr = to_write * 0x100;
        for (unsigned int i = 0; i < OAM_SIZE; i++) {
          cpu.oam[i] = gb_mem_ptr(cpu, dma_addr+i).read();
        }

        return;
      }
      case REG_BG_PALETTE:
        cpu.bg_palette = to_write;
        return;
      case REG_OBJ_PALETTE_0:
        cpu.obj_palette_0 = to_write;
        return;
      case REG_OBJ_PALETTE_1:
        cpu.obj_palette_1 = to_write;
        return;
      case REG_WINDOW_Y:
        cpu.window_y = to_write;
        return;
      case REG_WINDOW_X:
        cpu.window_x = to_write;
        return;
      default:
        break;
      }
    }

    // 0xfe00
    if ((OAM_BASE <= addr) &&
        (addr < OAM_BASE + OAM_SIZE)) {
      cpu.oam[addr - OAM_BASE] = to_write;
      return;
    }

    // 0xff80
    if ((HIGH_RAM_BASE <= addr) &&
        (addr < HIGH_RAM_BASE + HIGH_RAM_SIZE)) {
      cpu.highRam[addr - HIGH_RAM_BASE] = to_write;
      return;
    }

    // 0xffff
    if (addr == REG_INTERRUPT_ENABLE) {
      // COMPAT Are these masks correct? Unclear.
      cpu.interrupts_enabled = to_write & INT_ALL;
      return;
    }

    switch (cart_mbc_type(cpu)) {
    case MBC_NONE:
      break;
    case MBC_1:
    {
      if (addr < 0x2000) {
        // RAM enable
        // COMPAT: 0x0a enables RAM. Does anything else?
        cpu.expansionRamEnabled = bool(to_write);
        return;
      } else if (addr < 0x4000) {
        // ROM bank lower bits. Checking for zero here will prevent
        // accessing banks 0x20, 0x40, and 0x60
        to_write = to_write & 0x1f;
        if (!to_write) {
          to_write = 1;
        }
        cpu.rom_bank_low = to_write;
        return;
      } else if (addr < 0x6000) {
        // ROM bank upper bits or RAM bank
        cpu.ram_bank = to_write & 0x03;
        return;
      } else if (addr < 0x8000){
        cpu.mbc_mode = to_write & 1;
        return;
      }
      break;
    }
    case MBC_2:
    {
      if (addr < 0x2000) {
        // RAM enable
        // COMPAT: 0x0a enables RAM. Does anything else?
        cpu.expansionRamEnabled = bool(to_write);
        return;
      } else if (addr < 0x4000) {
        // COMPAT: I have no idea why this condition is here or what
        // happens if it isn't met
        if (addr & 0x0100) {
          // ROM bank
          to_write = to_write & 0x0f;
          if (!to_write) {
            to_write = 1;
          }
          cpu.rom_bank_low = to_write;
        } else if (MEM_WARN) {
          // let's just print a debug message if we fall through this
          // bizarre condition (addr & 0x0100)
          fprintf(stderr,
                  "Ignoring write to MBC2 ROM bank select because "
                  "bit 8 of address was unset??? (%02x -> %04x)",
                  to_write, addr);

        }
        return;
      }
      break;
    }
    case MBC_3:
    {
      if (addr < 0x2000) {
        // RAM and timer enable
        // COMPAT: 0x0a enables RAM. Does anything else?
        cpu.expansionRamEnabled = bool(to_write);
        return;
      } else if (addr < 0x4000) {
        // ROM bank (all bits).
        to_write &= 0x7f;
        if (!to_write) {
          to_write = 1;
        }
        cpu.rom_bank_low = to_write;
        return;
      } else if (addr < 0x6000) {
        // RAM bank or timer register select
        cpu.ram_bank = to_write & 0x0f;
        return;
      } else if (addr < 0x8000){
        // Timer latch
        // TODO
        return;
      }
      break;
    }
    default:
      fprintf(stderr, "Unimplemented MBC %d: Can't interpret write of %02x to %04x\n",
              cart_mbc_type(cpu), to_write, addr);
      exit(-1);
      // TODO other MBCs
      break;
    }

    if (MEM_WARN) {
      fprintf(stderr, "Ignoring write to unimplemented address %04x\n", addr);
    }
    return;

    // fprintf(stderr, "Write to unimplemented address %04x\n", addr);
    // exit(0);
  }
  case GB_PTR_REG:
  {
    *(val.reg) = to_write;
    return;
  }
  default:
    fprintf(stderr, "gb_ptr::write(): bad ptr_type %d\n", ptr_type);
    exit(0);
  }
}

void gb_ptr::write_16(uint16_t to_write) {
  assert(ptr_type == GB_PTR_MEM);
  gb_mem16_ptr(cpu, val.addr).write(to_write);
}

gb_ptr gb_ptr::operator+(int i) const {
  assert(ptr_type == GB_PTR_MEM);
  return gb_ptr(cpu, ptr_type,
                {.addr = static_cast<uint16_t>(val.addr + i)});
}

gb_ptr gb_ptr::operator-(int i) const {
  assert(ptr_type == GB_PTR_MEM);
  return gb_ptr(cpu, ptr_type,
                {.addr = static_cast<uint16_t>(val.addr + i)});
}

gb_ptr gb_mem_ptr(CPU &c, uint16_t addr) {
  return gb_ptr(c, GB_PTR_MEM, {.addr=addr});
}

gb_ptr gb_reg_ptr(CPU &c, uint8_t *reg) {
  return gb_ptr(c, GB_PTR_REG, {.reg=reg});
}

// END GB_PTR

// BEGIN GB_PTR_16

gb_ptr_16::gb_ptr_16(CPU &c, const gb_ptr_type t, const gb_ptr_16_val v)
  : cpu(c), ptr_type(t), val(v)
{
  assert(ptr_type < GB_PTR_TYPE_MAX);
}

uint16_t gb_ptr_16::read() {

  switch(ptr_type) {

  case GB_PTR_MEM:
  {
    const uint16_t addr = val.addr;
    uint8_t out_low = gb_ptr(cpu, GB_PTR_MEM,
                             {.addr=addr}).read();
    uint8_t out_high = gb_ptr(cpu, GB_PTR_MEM,
                              {.addr = static_cast<uint16_t>(addr+1)}
                                  ).read();
    return out_low + (out_high<<8);
  }
  case GB_PTR_REG:
  {
    return *(val.reg);
  }
  default:
    fprintf(stderr, "gb_ptr_16::read(): bad ptr_type %d\n", ptr_type);
    exit(0);
  }
}

void gb_ptr_16::write(uint16_t to_write) {

  switch(ptr_type) {

  case GB_PTR_MEM:
  {
    const uint16_t addr = val.addr;
    uint8_t in_low = to_write & 0xff;
    uint8_t in_high = to_write >> 8;
    gb_ptr(cpu, GB_PTR_MEM, {.addr=addr}).write(in_low);
    gb_ptr(cpu, GB_PTR_MEM,
           {.addr = static_cast<uint16_t>(addr+1)}
      ).write(in_high);
    return;
  }
  case GB_PTR_REG:
  {
    *(val.reg) = to_write;
    return;
  }
  default:
    fprintf(stderr, "gb_ptr_16::write(): bad ptr_type %d\n", ptr_type);
    exit(0);
  }
}

gb_ptr_16 gb_ptr_16::operator+(int i) const {
  assert(ptr_type == GB_PTR_MEM);
  return gb_ptr_16(cpu, ptr_type,
                   {.addr = static_cast<uint16_t>(val.addr + (i*2))});
}

gb_ptr_16 gb_ptr_16::operator-(int i) const {
  assert(ptr_type == GB_PTR_MEM);
  return gb_ptr_16(cpu, ptr_type,
                   {.addr = static_cast<uint16_t>(val.addr + (i*2))});
}

gb_ptr_16 gb_mem16_ptr(CPU &c, uint16_t addr) {
  return gb_ptr_16(c, GB_PTR_MEM, {.addr=addr});
}

gb_ptr_16 gb_reg16_ptr(CPU &c, uint16_t *reg) {
  return gb_ptr_16(c, GB_PTR_REG, {.reg=reg});
}

// END GB_PTR_16
