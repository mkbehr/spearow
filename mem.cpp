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

    if (HIGH_RAM_BASE <= addr) {
      // Test addr against edge of high ram with a static assert to
      // avoid -Wtautological-constant-out-of-range-compare complaining
      static_assert(
        std::numeric_limits< decltype(addr) >::max()
        < (int) HIGH_RAM_BASE + (int) HIGH_RAM_SIZE,
        "High ram should occupy the top of memory");
      return cpu.highRam[addr - HIGH_RAM_BASE];
    }

    fprintf(stderr, "Read from unimplemented address %04x\n", addr);
    exit(0);
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

    if (HIGH_RAM_BASE <= addr) {
      // Test addr against edge of high ram with a static assert to
      // avoid -Wtautological-constant-out-of-range-compare complaining
      static_assert(
        std::numeric_limits< decltype(addr) >::max()
        < (int) HIGH_RAM_BASE + (int) HIGH_RAM_SIZE,
        "High ram should occupy the top of memory");
      cpu.highRam[addr - HIGH_RAM_BASE] = to_write;
      return;
    }

    fprintf(stderr, "Write to unimplemented address %04x\n", addr);
    exit(0);
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

void gb_ptr_16::write(uint8_t to_write) {

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
