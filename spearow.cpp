#include <cassert>
#include <cstdint>
#include <cstdio>

#include <vector>
#include <algorithm> // std::sort
#include <numeric> // std::iota

#include "cpu.hpp"
#include "mem.hpp"
#include "opcodes.hpp"
#include "debugger.hpp"

void disassembleFunction(CPU &cpu, uint16_t addr) {
  uint8_t op_first;
  const char *opcode_name;
  do {
    op_first = gb_mem_ptr(cpu, addr).read();
    opcode_name = op_first == 0xcb ?
      CB_OPCODE_NAMES[(gb_mem_ptr(cpu, addr+1)).read()] :
      OPCODE_NAMES[op_first];
    printf("%04x: %s: %02x", addr, opcode_name, op_first);
    for (int i = 0; i < OPCODE_LENGTHS[op_first] - 1; i++) {
      uint8_t arg = gb_mem_ptr(cpu, addr+i+1).read();
      printf(" %02x", arg);
    }
    printf("\n");
    addr += OPCODE_LENGTHS[op_first];
  } while (strncmp(opcode_name, "RET", 3) != 0);
}

void runFiniteInstrs(CPU &cpu,
                     unsigned long long instrs,
                     int printStateEvery) {
  for (unsigned long long i = 0; i < instrs; i++) {
    cpu.tick();
    if (printStateEvery && (i % printStateEvery == 0)) {
      cpu.printState();
    }
  }
}

void printInstrCountTable() {
  std::vector<int> count_indices(256);
  std::iota(count_indices.begin(), count_indices.end(), 0);
  long *oc = opcode_counts; // something about lambdas and capturing
  std::sort(count_indices.begin(), count_indices.end(),
            [oc](int i, int j) {
              return oc[i] > oc[j];
            });
  printf("%6s | %12s | %6s\n",
         "OPCODE", "OPERATION", "COUNT");
  printf("-------+--------------+-------\n");
  for (int i = 0; i < 256; i++) {
    int opcode = count_indices.at(i);
    if (opcode_counts[opcode]) {
      printf("    %02x | %12s | %6ld \n",
             opcode,
             OPCODE_NAMES[opcode],
             opcode_counts[opcode]);
    }
  }
}

int main(int argc, char **argv) {
  CPU cpu;
  assert(argc > 1);
  cpu.loadRom(argv[1]);

  run_debugger(cpu);

  while (1) {
    cpu.tick();
  }
}
