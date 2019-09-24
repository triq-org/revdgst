/** @file
    utilh.: inlined util functions.

    Copyright (C) 2019 Christian W. Zuckschwerdt <zany@triq.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.
*/

#include <stdint.h>

__attribute__((always_inline))
static uint8_t reflect8(uint8_t x)
{
    x = (x & 0xF0) >> 4 | (x & 0x0F) << 4;
    x = (x & 0xCC) >> 2 | (x & 0x33) << 2;
    x = (x & 0xAA) >> 1 | (x & 0x55) << 1;
    return x;
}

__attribute__((always_inline))
static void reflect_bytes(uint8_t message[], unsigned num_bytes)
{
    for (unsigned i = 0; i < num_bytes; ++i) {
        message[i] = reflect8(message[i]);
    }
}

__attribute__((always_inline))
static uint8_t reflect4(uint8_t x)
{
    x = (x & 0xCC) >> 2 | (x & 0x33) << 2;
    x = (x & 0xAA) >> 1 | (x & 0x55) << 1;
    return x;
}

__attribute__((always_inline))
static void reflect_nibbles(uint8_t message[], unsigned num_bytes)
{
    for (unsigned i = 0; i < num_bytes; ++i) {
        message[i] = reflect4(message[i]);
    }
}

__attribute__((always_inline))
static uint8_t xor_bytes(uint8_t const message[], unsigned num_bytes)
{
    uint8_t result = 0;
    for (unsigned i = 0; i < num_bytes; ++i) {
        result ^= message[i];
    }
    return result;
}

__attribute__((always_inline))
static int add_bytes(uint8_t const message[], unsigned num_bytes)
{
    int result = 0;
    for (unsigned i = 0; i < num_bytes; ++i) {
        result += message[i];
    }
    return result;
}

__attribute__((always_inline))
static int add_nibbles(uint8_t const message[], unsigned num_bytes)
{
    int result = 0;
    for (unsigned i = 0; i < num_bytes; ++i) {
        result += (message[i] >> 4) + (message[i] & 0x0f);
    }
    return result;
}

__attribute__((always_inline))
static void invert_bytes(uint8_t message[], unsigned num_bytes)
{
    for (unsigned i = 0; i < num_bytes; ++i) {
        message[i] ^= 0xff;
    }
}

__attribute__((always_inline))
static uint8_t crc8(uint8_t const message[], unsigned nBytes, uint8_t polynomial, uint8_t init)
{
    uint8_t remainder = init;
    unsigned byte, bit;

    for (byte = 0; byte < nBytes; ++byte) {
        remainder ^= message[byte];
        for (bit = 0; bit < 8; ++bit) {
            if (remainder & 0x80) {
                remainder = (uint8_t)(remainder << 1) ^ polynomial;
            }
            else {
                remainder = (uint8_t)(remainder << 1);
            }
        }
    }
    return remainder;
}
