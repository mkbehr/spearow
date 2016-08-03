/* Debugger commands:
   - state: print CPU state
   - r addr [n]: read [n bytes of] memory at addr
   - w addr v: write value v to addr
   - step: step one CPU tick
   - break: add breakpoint
   - clear: remove breakpoint
   - cont: continue execution, stopping at breakpoints


   - exit: exit debugger
   - blank: repeat previous command
 */

#include <array>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>

#include "debugger.hpp"
#include "cpu.hpp"
#include "mem.hpp"

using namespace std;

vector<uint16_t> breakpoints;

void cmd_state(CPU &cpu, stringstream &cmdstream) {
  cpu.printState();
}

void cmd_read(CPU &cpu, stringstream &cmdstream) {
  // Read as hex by default
  int addr;
  cmdstream >> hex >> addr;
  if (!cmdstream) {
    printf("Couldn't read address\n");
    return;
  }
  if ((addr < 0) || (addr > 0xffff)) {
    printf("Bad address %x\n", addr);
    return;
  }
  int bytes;
  cmdstream >> dec >> bytes;
  if (!cmdstream) {
    bytes = 1;
  }
  cout << "["
       << hex << setw(4) << setfill('0') << addr
       << "] ";
  for (int i = 0; i < bytes; i++) {
    cout << hex << setw(2) << setfill('0')
         << (int) gb_mem_ptr(cpu, addr + i).read()
         << " ";
  }
  cout << "\n";
}

void cmd_write(CPU &cpu, stringstream &cmdstream) {
  int addr;
  cmdstream >> hex >> addr;
  if (!cmdstream) {
    printf("Couldn't read address\n");
    return;
  }
  if ((addr < 0) || (addr > 0xffff)) {
    printf("Bad address %x\n", addr);
    return;
  }

  int val;
  cmdstream >> hex >> val;
  if (!cmdstream) {
    printf("Couldn't read value\n");
    return;
  }
  if ((val < 0) || (val > 0xff)) {
    printf("Bad value %x\n", val);
    return;
  }

  gb_mem_ptr(cpu, addr).write(val);

  // confirm write
  cout << "["
       << hex << setw(4) << setfill('0') << addr
       << "] "
       << hex << setw(2) << setfill('0')
       << (int) gb_mem_ptr(cpu, addr).read()
       << "\n";
}

void cmd_step(CPU &cpu, stringstream &cmdstream) {
  cpu.tick();
  cpu.printState();
}

void cmd_break(CPU &cpu, stringstream &cmdstream) {
  int addr;
  cmdstream >> hex >> addr;
  if (!cmdstream) {
    printf("Couldn't read address\n");
    return;
  }
  if ((addr < 0) || (addr > 0xffff)) {
    printf("Bad address %x\n", addr);
    return;
  }
  if (find(breakpoints.begin(), breakpoints.end(),
           addr)
      != breakpoints.end()) {
    cout << "Breakpoint already set at "
         << hex << setw(4) << setfill('0') << addr
         << "\n";
    return;
  }
  breakpoints.push_back(addr);
  cout << "Set breakpoint at "
       << hex << setw(4) << setfill('0') << addr
       << "\n";
}

void cmd_clear(CPU &cpu, stringstream &cmdstream) {
  int addr;
  cmdstream >> hex >> addr;
  if (!cmdstream) {
    printf("Couldn't read address\n");
    return;
  }
  if ((addr < 0) || (addr > 0xffff)) {
    printf("Bad address %x\n", addr);
    return;
  }
  auto loc = find(breakpoints.begin(),
                  breakpoints.end(),
                  addr);
  if (loc == breakpoints.end()) {
    cout << "Breakpoint not found at "
         << hex << setw(4) << setfill('0') << addr
         << "\n";
    return;
  } else {
    breakpoints.erase(loc);
    cout << "Removed breakpoint at "
         << hex << setw(4) << setfill('0') << addr
         << "\n";
  }
}

void cmd_continue(CPU &cpu, stringstream &cmdstream) {
  cout << "Continuing\n";
  do {
    cpu.tick();
  } while (find(breakpoints.begin(), breakpoints.end(), cpu.pc)
           == breakpoints.end());
  cpu.printState();
}

const struct {string name; void (*cmd)(CPU &, stringstream &);} cmds[] = {
  {"state", cmd_state},
  {"r", cmd_read},
  {"w", cmd_write},
  {"step", cmd_step},
  {"break", cmd_break},
  {"clear", cmd_clear},
  {"cont", cmd_continue},
};

void run_debugger(CPU &cpu) {
  cpu.printState();
  string last_cmd;
  while (1) {
    string cmd_full;
    cout << "> ";
    getline(cin, cmd_full);
    if (!cin) {
      break;
    }
    if (cmd_full.empty()) {
      cmd_full = last_cmd;
    }
    last_cmd = cmd_full;
    stringstream cmdstream = stringstream(cmd_full);
    string cmd;
    cmdstream >> cmd;

    bool recognized = 0;

    if (cmd.compare("exit") == 0) {
      break;
    }

    for (int i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++){
      if (cmd.compare(cmds[i].name) == 0) {
        cmds[i].cmd(cpu, cmdstream);
        recognized = 1;
        break;
      }
    }

    if (!recognized) {
      cout << "Unrecognized command\n";
    }
  }
  cout << "\n";
}
