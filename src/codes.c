/** @file
    codes: read a wide variety of code data files

    Copyright (C) 2019 Christian W. Zuckschwerdt <zany@triq.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.
*/

#include "codes.h"

int read_codes(char const *filename, struct data *data, unsigned *msg_len, unsigned msg_max, unsigned list_max)
{
    FILE *fp;
    char line[LINE_MAX];
    int multiline_comment = 0;
    unsigned cnt = 0;
    unsigned c_len = 0;

    if (filename && *filename) {
        fp = fopen(filename, "r");
    } else {
        fp = stdin;
    }
    if (!fp) {
        fprintf(stderr, "Failed to open %s\n", filename);
        return -1;
    }

    while (fgets(line, LINE_MAX, fp)) {
        // parse hex chars
        uint8_t *d = data->d;
        unsigned nibble = 0;
        for (char *p = line; *p; ++p) {
            // skip multiline comment
            if (*p == '/' && p[1] == '*') {
                multiline_comment = 1;
                p += 2;
            }
            if (multiline_comment) {
                while (*p && (*p != '*' || p[1] != '/'))
                    p++;
                if (*p == '*' && p[1] == '/') {
                    multiline_comment = 0;
                    p++;
                }
                if (!*p)
                    continue;
            }

            int digit;
            // end at comments
            if (*p == ';' || *p == '#')
                break;
            if (*p == '/' && p[1] == '/') {
                p++;
                break;
            }
            // skip '0x'
            if (*p == '0' && p[1] == 'x') {
                p++;
                continue;
            }
            if ((*p < '0' || *p > '9') && (*p < 'A' || *p > 'F') && (*p < 'a' || *p > 'f'))
                continue;

            sscanf(p, "%1x", &digit);
            if (nibble & 1) {
                *d++ |= digit; // low order nibble
            }
            else {
                *d = digit << 4; // high order nibble
            }
            nibble++;
            if (nibble / 2 >= msg_max) {
                fprintf(stderr, "Maximum number of msg bytes (%u) reached.\n", msg_max);
                break;
            }
        }
        if (nibble > 0) {
            if (!c_len) {
                c_len = (nibble + 1) / 2;
                fprintf(stderr, "Code len is %u bytes (%u nibbles), ", c_len, nibble);
            }
            else if (c_len != (nibble + 1) / 2) {
                fprintf(stderr, "Code len mismatched %u bytes expected but got %u bytes (%u nibbles).\n", c_len, (nibble + 1) / 2, nibble);
            }
            data->chk = d[-1];
            data->chk16 = (d[-2] << 8) | d[-1];
            data++;

            cnt++;
            if (cnt >= list_max) {
                fprintf(stderr, "Maximum number of input lines (%u) reached.\n", list_max);
                break;
            }
        }
    }
    fprintf(stderr, "%u codes read.\n", cnt);

    if (filename && *filename) {
        fclose(fp);
    }
    *msg_len = c_len;
    return cnt;
}

int sprint_code(char *dst, struct data *data, unsigned msg_len)
{
    int r = 0;
    for (unsigned j = 0; j < msg_len; ++j) {
        r += sprintf(dst, "%02x ", data->d[j]);
        dst += 3;
    }
    return r;
}

void print_codes(struct data *data, unsigned msg_len, unsigned list_len)
{
    char buf[MSG_MAX * 3 + 1];
    printf("; %u code of len %u\n", list_len, msg_len);
    for (unsigned i = 0; i < list_len; ++i) {
        sprint_code(buf, data, msg_len);
        //printf("%s ; %02x %04x\n", buf, data->chk, data->chk16);
        printf("%s\n", buf);
        data++;
    }
}
