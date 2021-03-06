/* Debugger commands:
   - state: print CPU state
   - r addr [n]: read [n bytes of] memory at addr
   - w addr v: write value v to addr
   - step: step one CPU tick
   - break: add breakpoint
   - clear: remove breakpoint
   - cont: continue execution, stopping at breakpoints
   - shutdown: stop emulator


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
#include "opcodes.hpp"
#include "mem.hpp"

using namespace std;

// TODO fix include discipline so we can put this in the header
// without linker errors
bool DEBUG_SHUTDOWN_ON_EOF = 1;

vector<uint16_t> breakpoints;

bool read_addr(stringstream &cmdstream, uint16_t &addr){
  int out;
  cmdstream >> hex >> out;
  if (!cmdstream) {
    printf("Couldn't read address\n");
    return 0;
  }
  if ((out < 0) || (out > 0xffff)) {
    printf("Bad address 0x%x\n", out);
    return 0;
  }
  addr = out;
  return 1;
}

void cmd_state(CPU &cpu, stringstream &cmdstream) {
  cpu.printState();
}

void cmd_interrupts(CPU &cpu, stringstream &cmdstream) {
  cout << "Interrupts "
       << (cpu.interrupt_master_enable ? "enabled" : "disabled")
       << "\n";
  cout << "Enabled interrupts:   "
       << hex << setw(2) << setfill('0') << (int) cpu.interrupts_enabled
       << "\n";
  cout << "Requested interrupts: "
       << hex << setw(2) << setfill('0') << (int) cpu.interrupts_raised
       << "\n";
  printf("%02x %02x\n", cpu.interrupts_enabled, cpu.interrupts_raised);
}

void cmd_read(CPU &cpu, stringstream &cmdstream) {
  // Read as hex by default
  uint16_t addr;
  if (!read_addr(cmdstream, addr)) {
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
  uint16_t addr;
  if (!read_addr(cmdstream, addr)) {
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

void cmd_wreg(CPU &cpu, stringstream &cmdstream) {
  string regname;
  cmdstream >> regname;
  if (!cmdstream) {
    printf("Couldn't read register name\n");
    return;
  }

  int val;
  cmdstream >> hex >> val;
  if (!cmdstream) {
    printf("Couldn't read value\n");
    return;
  }

  struct {string name; gb_ptr ptr;} regs_8[] = {
    {"b", gb_reg_ptr(cpu, &cpu.bc.high)},
    {"c", gb_reg_ptr(cpu, &cpu.bc.low)},
    {"d", gb_reg_ptr(cpu, &cpu.de.high)},
    {"e", gb_reg_ptr(cpu, &cpu.de.low)},
    {"a", gb_reg_ptr(cpu, &cpu.af.high)},
    {"f", gb_reg_ptr(cpu, &cpu.af.low)},
    {"h", gb_reg_ptr(cpu, &cpu.hl.high)},
    {"l", gb_reg_ptr(cpu, &cpu.hl.low)},
  };
  struct {string name; gb_ptr_16 ptr;} regs_16[] = {
    {"af", gb_reg16_ptr(cpu, &cpu.af.full)},
    {"bc", gb_reg16_ptr(cpu, &cpu.bc.full)},
    {"de", gb_reg16_ptr(cpu, &cpu.de.full)},
    {"hl", gb_reg16_ptr(cpu, &cpu.hl.full)},
    {"pc", gb_reg16_ptr(cpu, &cpu.pc)},
    {"sp", gb_reg16_ptr(cpu, &cpu.sp)},
  };

  bool recognized = 0;

  for (int i = 0; i < sizeof(regs_8) / sizeof(regs_8[0]); i++){
    if (regname.compare(regs_8[i].name) == 0) {
      if ((val < 0) || (val > 0xff)) {
        cout << "Bad value " << hex << val << " for register " << regname << "\n";
        return;
      }
      regs_8[i].ptr.write(val);
      recognized = 1;
      break;
    }
  }

  if (!recognized) {
    for (int i = 0; i < sizeof(regs_16) / sizeof(regs_16[0]); i++){
      if (regname.compare(regs_16[i].name) == 0) {
        if ((val < 0) || (val > 0xffff)) {
          cout << "Bad value " << hex << val << " for register " << regname << "\n";
          return;
        }
        regs_8[i].ptr.write(val);
        recognized = 1;
        break;
      }
    }
  }

  if (recognized) {
    // confirm write
    cpu.printState();
  } else {
    cout << "Unrecognized register\n";
  }
}

void cmd_step(CPU &cpu, stringstream &cmdstream) {
  cpu.tick();
  cpu.printState();
}

void cmd_break(CPU &cpu, stringstream &cmdstream) {
  uint16_t addr;
  if (!read_addr(cmdstream, addr)) {
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
  uint16_t addr;
  if (!read_addr(cmdstream, addr)) {
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
  cpu.install_sigint();
  do {
    cpu.tick();
  } while (find(breakpoints.begin(), breakpoints.end(), cpu.pc)
           == breakpoints.end());
  cpu.uninstall_sigint();
  cpu.printState();
}

void cmd_shutdown(CPU &cpu, stringstream &cmdstream) {
  exit(0);
}

// tilea: print tile data from an address
void cmd_tilea(CPU &cpu, stringstream &cmdstream) {
  uint16_t base;
  if (!read_addr(cmdstream, base)) {
    return;
  }
  for (int row = 0; row < 8; row++) {
    uint8_t low_byte = gb_mem_ptr(cpu, base + row*2).read();
    uint8_t high_byte = gb_mem_ptr(cpu, base + row*2 + 1).read();
    for (int col = 0; col < 8; col++) {
      int out = 0;
      if (low_byte & (1<<(7-col))) {
        out += 1;
      }
      if (high_byte & (1<<(7-col))) {
        out += 2;
      }
      if (out) {
        cout << out;
      } else {
        cout << '.';
      }
    }
    cout << '\n';
  }
}

void cmd_ascii_screen(CPU &cpu, stringstream &cmdstream) {
  // Testing out some rendering code here before I move it elsewhere.

  // For testing purposes, REG_LCD_CONTROL stores 0x91, so:
  // BG on
  // Sprites off
  // Sprites 8x8
  // BG code starts at 0x9800
  // BG tiles start at 0x8000
  // Window off
  // Window code starts at 0x9800
  // Screen on
  for (int y = 0; y < 144; y++) {
    int tileY = y / 8;
    for (int x = 0; x < 160; x++) {
      int tileX = x / 8;
      int block = tileY * 32 + tileX;

      uint16_t bg_base = 0x9800; // TODO confirm
      uint8_t tile_n = gb_mem_ptr(cpu, bg_base + block).read();

      uint16_t tile_base = 0x8000;

      uint16_t tile_addr = tile_base + tile_n * 16;

      uint8_t low_byte = gb_mem_ptr(cpu, tile_addr + (y%8)*2).read();
      uint8_t high_byte = gb_mem_ptr(cpu, tile_addr + (y%8)*2 + 1).read();

      int out = 0;
      if (low_byte & (1<<(7-(x%8)))) {
        out += 1;
      }
      if (high_byte & (1<<(7-(x%8)))) {
        out += 2;
      }
      if (out) {
        cout << out;
      } else {
        cout << '.';
      }
    }
    cout << "\n";
  }
  cout << "\n";
}

void cmd_draw(CPU &cpu, stringstream &cmdstream) {
  cpu.screen->draw();
}

void cmd_disasssemble_fun(CPU &cpu, stringstream &cmdstream) {
  uint16_t addr;
  if (!read_addr(cmdstream, addr)) {
    return;
  }

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
    // stop on return, absolute jump, or reset
    // (will miss interrupts triggered by writing to the interrupt register)
  } while ((strncmp(opcode_name, "RET", 3) != 0) &&
           (strncmp(opcode_name, "JR", 2) != 0) &&
           (strncmp(opcode_name, "JP", 2) != 0) &&
           (strncmp(opcode_name, "RST", 2) != 0) &&
           (OPCODE_LENGTHS[op_first] != 0));
}

const struct {string name; void (*cmd)(CPU &, stringstream &);} cmds[] = {
  {"state", cmd_state},
  {"read", cmd_read},
  {"r", cmd_read},
  {"write", cmd_write},
  {"w", cmd_write},
  {"wreg", cmd_wreg},
  {"step", cmd_step},
  {"s", cmd_step},
  {"break", cmd_break},
  {"b", cmd_break},
  {"clear", cmd_clear},
  {"cont", cmd_continue},
  {"c", cmd_continue},
  {"shutdown", cmd_shutdown},
  {"sd", cmd_shutdown},
  {"tilea", cmd_tilea},
  {"screen", cmd_ascii_screen},
  {"draw", cmd_draw},
  {"disassemble_fun", cmd_disasssemble_fun},
  {"interrupts", cmd_interrupts},
};

void run_debugger(CPU &cpu) {
  cpu.printState();
  string last_cmd;
  while (1) {
    string cmd_full;
    cout << "> ";
    getline(cin, cmd_full);
    if (!cin) {
      if (DEBUG_SHUTDOWN_ON_EOF) {
        cout << "\n";
        exit(0);
      }
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
