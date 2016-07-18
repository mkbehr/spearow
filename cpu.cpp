#include "cpu.hpp"

CPU::CPU()
  : af({.full=0}), bc({.full=0}), de({.full=0}),
    sp(INITIAL_SP), pc(INITIAL_PC)
{
}
