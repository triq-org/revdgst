# revdgst

Collection of various tools to analyze and reverse engineer checksums in short data packets.

Mainly intended to help analyze software defined radio (SDR) data transmission from sensor modules.

## Building / Installation

The main tools are written in portable C (C11 standard) and known to compile on Linux, MacOS, and Windows systems.

```
mkdir build
cd build
cmake ..
make
```

## Work in progress

Currently missing proper options, error checking, and documentation.
If this is somewhat useful to you, let me know, I might develop this further.

## Inputs

Input files need to contain hex messages.
A hex prefix ( `0x` ) will be ignored.
All padding characters will be ignored.
All codes need to be the same length (bytes or nibbles).
Inline-comments ( `;`, `#`, `//` ) will end the line.
Multi-line comments ( `/*` .. `*/`) will be skipped.

If no file is given stdin will be read.

## keylst

List keys from LFSR generators.

### bitbrk

Analyze and break out bit changes and compare checksums.

### revdgst

Reverse 8-bit LFSR digest.

Includes Galois, Fibonacci, Reverse-Galois, Reverse-Fibonacci, Fletcher, Reverse-Fletcher, Shift16.
Each on plain data, byte-reflect, bit-reflect, bit-reflect and byte-reflect.

### revdgst16

Reverse 16-bit LFSR digest.

### revsum

Reverse simple checksums.

Includes byte-wide sums, nibble-wide sum, parity, and CRC-8.

## Copyright and Licence

Copyright (C) 2019 Christian W. Zuckschwerdt <zany@triq.net>

Unless otherwise noted all sources are:

License: GPL-2+
