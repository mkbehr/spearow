#include <cstdint>
#include <cstdio>

#include <vector>
#include <algorithm> // std::sort
#include <numeric> // std::iota

#include "cpu.hpp"
#include "mem.hpp"
#include "opcodes.hpp"

// const char *TEST_ROMPATH="cpu_instrs.gb";
// const uint16_t PRINT_STR_HL_ADDR = 0x02d7;
// const uint16_t CONSOLE_WAIT_VBL_LOOP = 0x073e;

const char *TEST_ROMPATH="cpu_instrs/individual/03-op sp,hl.gb";
const uint16_t INIT_RUNTIME_ADDR = 0xcb0c;
const uint16_t ABOUT_TO_DELAY_ADDR = 0xc252;
const uint16_t AFTER_DELAY_ADDR = 0xc263;
const uint16_t PRINT_STR_HL_ADDR = 0x02d7;
const uint16_t CONSOLE_WAIT_VBL_LOOP = 0x073e;
// How I found this:
// - test cart boots up, copies a bunch of instructions to 0xc000,
//   then jumps there.
// - once at c000, it'll disable interrupts, do some loads, and then
//   call init_printing.
// - init_printing does an ld a,l and then ld (print_char_nocrc+1),a.
//   get print_char_nocrc from that.
const uint16_t PRINT_CHAR_NOCRC = 0xd801;


const int MAX_CHARS_TO_PRINT = 100;



void printStrAtHL(CPU &cpu) {
  fprintf(stdout, "Output string at %04x:\n",
          cpu.hl.full);
  uint16_t addr = cpu.hl.full;
  for (int i = 0; i < MAX_CHARS_TO_PRINT; i++) {
    uint8_t c = gb_mem_ptr(cpu, addr).read();
    if (!c) {
      break;
    }
    fprintf(stdout, "%c", c);
    addr++;
  }
  fflush(stdout);
}

void runFiniteInstrs(CPU &cpu, int instrs, int printStateEvery) {
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
  cpu.loadRom(TEST_ROMPATH);
  //cpu.printState();

  // runFiniteInstrs(cpu, 10000, 100);
  // return 0;

  for (unsigned long long i = 0; i < 1000000; i++) {
    cpu.tick();
    if (cpu.pc == PRINT_CHAR_NOCRC) {
      printf("%c", cpu.af.high);
      // cpu.printState();
      // break;
    }
    // if (cpu.pc == 0xc000) {
    //   cpu.printState();
    //   break;
    // }
    if (cpu.pc == ABOUT_TO_DELAY_ADDR) {
      printf("Beginning delay\n");
      //cpu.pc = AFTER_DELAY_ADDR;
      //break;
    }
    if (cpu.pc == AFTER_DELAY_ADDR) {
      printf("Finished delay\n");
    }
  }

  printInstrCountTable();

  //runFiniteInstrs(cpu, 1000000, 0);

  return 0;

  // runFiniteInstrs(cpu, 100, 0);
  // return 0;

  // for (int i = 0; i < 20; i++) {
  //   cpu.tick();
  //   cpu.printState();
  // }
  return 0;

  int n_vblanks = 0;

  for (unsigned long long i = 0; ; i++) {
    cpu.tick();
    if (cpu.pc == PRINT_STR_HL_ADDR) {
      printStrAtHL(cpu);
    } else if (cpu.pc == CONSOLE_WAIT_VBL_LOOP) {
      printf("Skipping vblank wait loop: %d\n",
             n_vblanks++);
      cpu.bc.full = -1; // we'll increment it next
      // break;
    }
    // if (i % 10000000 == 0) {
    //   cpu.printState();
    // }
  }

  return 0;
}
