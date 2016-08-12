#include <cassert>
#include <cstdint>
#include <cstdio>

#include <vector>
#include <algorithm> // std::sort
#include <numeric> // std::iota

#include <getopt.h> // getopt_long

#include "cpu.hpp"
#include "mem.hpp"
#include "opcodes.hpp"
#include "debugger.hpp"

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
  int debug = 0;

  static struct option opts[] = {
    {"debug", no_argument, &debug, 1},
  };


  int c;
  while ((c = getopt_long(argc, argv, "f:", opts, NULL)) != -1) {
    switch (c) {
    case 0:
      break;
    case '?':
    default:
      abort();
    }
  }

  char *rompath = argv[optind+0];

  CPU cpu;
  cpu.loadRom(rompath);

  if (debug) {
    cpu.uninstall_sigint();
    run_debugger(cpu);
  }

  while (1) {
    cpu.tick();
  }
}
