#include "cpu.hpp"

const char *TEST_ROMPATH="cpu_instrs.gb";

int main(int argc, char **argv) {
  CPU cpu;
  cpu.loadRom(TEST_ROMPATH);
  cpu.printState();
  for (int i = 0; i < 10; i++) {
    cpu.tick();
    cpu.printState();
  }
  return 0;
}
