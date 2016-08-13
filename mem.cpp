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
  default:
    // TODO
    return 0;
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

    if ((ROM_BASE <= addr) &&
        (addr < ROM_BASE + ROM_BANK_SIZE)) {

      return cpu.rom.at(addr - ROM_BASE);
    }

    if ((ROM_SWITCHABLE_BASE <= addr) &&
        (addr < ROM_SWITCHABLE_BASE + ROM_BANK_SIZE)) {
      return cpu.rom.at(addr - ROM_SWITCHABLE_BASE + rom_bank_offset(cpu));
    }

    if ((VRAM_BASE <= addr) &&
        (addr < VRAM_BASE + VRAM_SIZE)) {
      return cpu.vram[addr - VRAM_BASE];
    }

    if ((RAM_BASE <= addr) &&
        (addr < RAM_BASE + RAM_SIZE)) {
      return cpu.ram[addr - RAM_BASE];
    }

    if ((RAM_ECHO_BASE <= addr) &&
        (addr <= RAM_ECHO_TOP)) {
      switch (cart_mbc_type(cpu)) {
      case MBC_NONE:
        return cpu.ram[addr - RAM_ECHO_BASE];
      case MBC_1:// TODO implement external ram
        //return externalram[addr - RAM_ECHO_BASE + ram_echo_offset(cpu)];
        break;
      default:
        break;
      }
    }

    // COMPAT: the official gameboy programming manual suggests that
    // the unused addresses here might just be regular RAM? Unclear.
    if ((IO_BASE <= addr) &&
        (addr < IO_BASE + IO_SIZE)) {
      switch (addr) {
        // misc
      case REG_JOYPAD:
        break;
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
        // TODO sound
        // display
      case REG_LCD_CONTROL:
        return cpu.lcd_control;
      case REG_LCD_STATUS:
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

    if ((HIGH_RAM_BASE <= addr) &&
        (addr < HIGH_RAM_BASE + HIGH_RAM_SIZE)) {
      return cpu.highRam[addr - HIGH_RAM_BASE];
    }

    if (addr == REG_INTERRUPT_ENABLE) {
      return cpu.interrupts_enabled;
    }

    if (MEM_WARN) {
      fprintf(stderr, "Reading 0 from unimplemented address %04x\n", addr);
    }
    return 0;

    // fprintf(stderr, "Read from unimplemented address %04x\n", addr);
    // exit(0);
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

    if ((VRAM_BASE <= addr) &&
        (addr < VRAM_BASE + VRAM_SIZE)) {
      cpu.vram[addr - VRAM_BASE] = to_write;
      return;
    }

    // TODO: respect switchable RAM bank
    if ((RAM_BASE <= addr) &&
        (addr < RAM_BASE + RAM_SIZE)) {
      cpu.ram[addr - RAM_BASE] = to_write;
      return;
    }

    if ((RAM_ECHO_BASE <= addr) &&
        (addr <= RAM_ECHO_TOP)) {
      switch (cart_mbc_type(cpu)) {
      case MBC_NONE:
        cpu.ram[addr - RAM_ECHO_BASE] = to_write;
        return;
      case MBC_1:// TODO implement external ram
        //return externalram[addr - RAM_ECHO_BASE + ram_echo_offset(cpu)];
        break;
      default:
        break;
      }
    }


    if ((IO_BASE <= addr) &&
        (addr < IO_BASE + IO_SIZE)) {
      switch (addr) {
      case REG_JOYPAD:
        break;
      case REG_SERIAL_DATA:
        if (MONITOR_LINK_PORT) {
          printf("%c", to_write);
          fflush(stdout);
        }
        break;
      case REG_SERIAL_CONTROL:
        break;
      case REG_DIVIDER:
        cpu.fine_divider = 0; // ignore given value
        break;
      case REG_TIMER_COUNT:
        cpu.timer_count = to_write;
        break;
      case REG_TIMER_MOD:
        cpu.timer_mod = to_write;
        break;
      case REG_TIMER_CONTROL:
        cpu.timer_control = to_write;
        break;
      case REG_INTERRUPT:
        // COMPAT Are these masks correct? Unclear.
        cpu.interrupts_raised = to_write & INT_ALL;
        break;
      // Video registers
      // COMPAT Are any of these masked?
      case REG_LCD_CONTROL:
        cpu.lcd_control = to_write;
        if (!(cpu.lcd_control & 0x80)) {
          cpu.reset_lcd();
        }
        break;
      case REG_LCD_STATUS:
        cpu.lcd_status = to_write;
        break;
      case REG_SCROLL_Y:
        cpu.scroll_y = to_write;
        break;
      case REG_SCROLL_X:
        cpu.scroll_x = to_write;
        break;
      case REG_LCD_Y: // read-only
        break;
      case REG_LCD_Y_COMPARE:
        cpu.lcd_y_compare = to_write;
        break;
      case REG_DMA:
        // TODO
        break;
      case REG_BG_PALETTE:
        cpu.bg_palette = to_write;
        break;
      case REG_OBJ_PALETTE_0:
        cpu.obj_palette_0 = to_write;
        break;
      case REG_OBJ_PALETTE_1:
        cpu.obj_palette_1 = to_write;
        break;
      case REG_WINDOW_Y:
        cpu.window_y = to_write;
        break;
      case REG_WINDOW_X:
        cpu.window_x = to_write;
        break;
      default:
        break;
      }
    }

    if ((HIGH_RAM_BASE <= addr) &&
        (addr < HIGH_RAM_BASE + HIGH_RAM_SIZE)) {
      cpu.highRam[addr - HIGH_RAM_BASE] = to_write;
      return;
    }

    if (addr == REG_INTERRUPT_ENABLE) {
      // COMPAT Are these masks correct? Unclear.
      cpu.interrupts_enabled = to_write & INT_ALL;
    }

    switch (cart_mbc_type(cpu)) {
    case MBC_NONE:
      break;
    case MBC_1:
      if (addr < 0x2000) {
        // RAM enable
        fprintf(stderr, "Ignoring write to RAM enable\n");
      } else if (addr < 0x4000) {
        // ROM bank lower bits
        to_write = to_write & 0x1f;
        if (!to_write) {
          to_write = 1;
        }
        cpu.rom_bank_low = to_write;
        return;
      } else if (addr < 0x6000) {
        // ROM bank upper bits or RAM bank
        cpu.ram_bank = to_write & 0x03;
      } else if (addr < 0x8000){
        cpu.mbc_mode = to_write & 1;
      }
      break;
    default:
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
