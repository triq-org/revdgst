#!/usr/bin/env python2.7

"""Differential Manchester decode string."""

from __future__ import print_function

from bitstring import BitArray

__author__ = "Christian W. Zuckschwerdt"
__copyright__ = "Copyright 2018, Christian W. Zuckschwerdt"
__license__ = "GPLv2+"
__version__ = "1.0.0"
__maintainer__ = "Christian W. Zuckschwerdt"
__email__ = "zany@triq.net"
__status__ = "Production"


def differential_manchester_align(line):
    """Manchester align string by optionally prepending a half-bit."""
    hh_pos = line.find("11")
    ll_pos = line.find("00")

    if hh_pos < 0 or (ll_pos >= 0 and ll_pos < hh_pos):
        hh_pos = ll_pos

    if hh_pos % 2 == 0:
        return line
    else:
        return "0" + line


def differential_manchester_decode(line):
    """Differential Manchester decode string."""
    i = 0
    length = len(line)

    buf = ""
    while i < length - 1:
        if line[i] == line[i + 1]:
            buf += '1'
        else:
            buf += '0'

        i += 2

    return buf


def strip_length(line):
    """Strip optional length indicator."""
    if "}" in line:
        return line[line.find("}") + 1:]
    else:
        return line


def process_all(lines):
    """Process all lines."""
    for line in lines:
        line = strip_length(line)
        if len(line.replace('0', '').replace('1', '')) > 0:
            line = BitArray('0x' + line).bin[2:]
        code = differential_manchester_decode(differential_manchester_align(line))
        if code:
            for c in code.split():
                print(c, hex(int(c, 2)))


if __name__ == '__main__':
    import sys
    process_all(sys.argv[1:])
