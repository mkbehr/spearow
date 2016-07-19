#ifndef CPU_H

#define CPU_H

#ifndef __BYTE_ORDER__
#error Unknown byte order. Set __BYTE_ORDER__ to the appropriate value.
#endif

typedef union register_pair {
  struct {
    // Set struct ordering according to machine endianness.
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) || \
  (__BYTE_ORDER__ == __ORDER_PDP_ENDIAN__)
    unsigned char low;
    unsigned char high;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN
    unsigned char high;
    unsigned char low;
#else
#error Unrecognized byte order!
#endif
  };
  unsigned int full:16;
} register_pair;

const int FLAG_Z = 1 << 7; // zero flag
const int FLAG_N = 1 << 6; // subtract flag
const int FLAG_H = 1 << 5; // half-carry flag
const int FLAG_C = 1 << 4; // carry flag

const unsigned int INITIAL_SP = 0xfffe;
const unsigned int INITIAL_PC = 0x0100;

class CPU {
public:
  CPU();

  void updateFlags(int z, int n, int h, int c); // TODO implementation

  register_pair af;
  register_pair bc;
  register_pair de;
  register_pair hl;
  unsigned int sp:16; // stack pointer
  unsigned int pc:16; // program counter
};

#endif // #ifndef CPU_H
