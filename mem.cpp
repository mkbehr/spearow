#include <cassert>
#include <cstdio>
#include <cstdlib>

#include "mem.hpp"

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
    // TODO: respect switchable ROM bank
    if ((ROM_BASE <= addr) &&
        (addr < ROM_SWITCHABLE_BASE + ROM_BANK_SIZE)) {

      return cpu.rom.at(addr - ROM_BASE);
    }

    // TODO: respect switchable RAM bank
    if ((RAM_BASE <= addr) &&
        (addr < RAM_BASE + RAM_SIZE)) {
      return cpu.ram[addr - RAM_BASE];
    }

    if ((RAM_ECHO_BASE <= addr) &&
        (addr <= RAM_ECHO_TOP)) {
      return cpu.ram[addr - RAM_ECHO_BASE];
    }

    if ((IO_BASE <= addr) &&
        (addr < IO_BASE + IO_SIZE)) {
      switch (addr) {
      case REG_JOYPAD:
        break;
      case REG_SERIAL_DATA:
        break;
      case REG_SERIAL_CONTROL:
        break;
      case REG_DIVIDER:
        break;
      case REG_TIMER_COUNT:
        break;
      case REG_TIMER_MOD:
        break;
      case REG_TIMER_CONTROL:
        break;
      case REG_INTERRUPT:
        return cpu.interrupts_raised;
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

    // TODO: respect switchable RAM bank
    if ((RAM_BASE <= addr) &&
        (addr < RAM_BASE + RAM_SIZE)) {
      cpu.ram[addr - RAM_BASE] = to_write;
      return;
    }

    if ((RAM_ECHO_BASE <= addr) &&
        (addr <= RAM_ECHO_TOP)) {
      cpu.ram[addr - RAM_ECHO_BASE] = to_write;
      return;
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
        break;
      case REG_TIMER_COUNT:
        break;
      case REG_TIMER_MOD:
        break;
      case REG_TIMER_CONTROL:
        break;
      case REG_INTERRUPT:
        // COMPAT Are these masks correct? Unclear.
        cpu.interrupts_raised = to_write & INT_ALL;
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
