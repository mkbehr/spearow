#ifndef MEM_H

#define MEM_H

#include "cpu.hpp"

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
  void write(uint8_t);
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
  void write(uint8_t);
  gb_ptr_16 operator+(int) const;
  gb_ptr_16 operator-(int) const;
private:
  CPU &cpu;
  const gb_ptr_type ptr_type;
  const gb_ptr_16_val val;

};

gb_ptr gb_mem_ptr(CPU &, uint16_t);
gb_ptr_16 gb_mem16_ptr(CPU &, uint16_t);

#endif // $ifndef MEM_H
