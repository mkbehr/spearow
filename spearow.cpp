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

void usage(int argc, char **argv, struct option *opts) {
  const char *programname = "spearow";
  if (argc > 0) {
    programname = argv[0];
  }

  // TODO short args (if we ever have any)

  printf("usage: %s", programname);
  for (int i = 0; opts[i].name; i++) {
    printf(" [--%s]", opts[i].name);
  }

  printf(" file\n");
}

int main(int argc, char **argv) {
  int debug = 0;
  int displayTiles = 0;

  static struct option opts[] = {
    {"help", no_argument, NULL, 'h'},
    {"debug", no_argument, &debug, 1},
    {"display-tiles", no_argument, &displayTiles, 1},
    {0, 0, 0, 0}
  };


  int c, option_index;
  while ((c = getopt_long(argc, argv, "f:", opts, &option_index)) != -1) {
    switch (c) {
    case 0:
      break;
    case ':':
    case '?':
    default:
      usage(argc, argv, opts);
      exit(-1);
    }
  }

  if (optind >= argc) {
    usage(argc, argv, opts);
    exit(-1);
  }

  char *rompath = argv[optind+0];

  CPU cpu(displayTiles);
  cpu.loadRom(rompath);

  if (debug) {
    cpu.uninstall_sigint();
    run_debugger(cpu);
  }

  while (1) {
    cpu.tick();
  }
}
