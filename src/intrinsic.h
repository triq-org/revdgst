/** @file
    intrinsic.h: inlined intrinsics.

    Copyright (C) 2019 Christian W. Zuckschwerdt <zany@triq.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.
*/

#ifdef __GNUC__

__attribute__((always_inline))
static inline int popcount(unsigned x)
{
    return __builtin_popcount(x);
}

__attribute__((always_inline))
static inline int parity(unsigned x)
{
    return __builtin_parity(x);
}

__attribute__((always_inline))
static inline int parityll(unsigned x)
{
    return __builtin_parityll(x);
}

#else
#warning No builtin popcount

__attribute__((always_inline))
static inline int popcount(unsigned x)
{
    x = x - ((x >> 1) & 0x55555555);
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    return (((x + (x >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

__attribute__((always_inline))
static inline int parity(unsigned x)
{
    x = (x & 0x55555555) + ((x >> 1) & 0x55555555);
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    x = (x & 0x0F0F0F0F) + ((x >> 4) & 0x0F0F0F0F);
    x = (x & 0x00FF00FF) + ((x >> 8) & 0x00FF00FF);
    x = (x & 0x0000FFFF) + ((x >> 16) & 0x0000FFFF);
    return x & 1;
}

__attribute__((always_inline))
static inline int parityll(unsigned long long x)
{
    return parity(x ^ (x >> 32));
}

#endif
