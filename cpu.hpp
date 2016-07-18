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
  } small;
  int big:16;
} register_pair;

#endif // #ifndef CPU_H
