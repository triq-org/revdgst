/** @file
    codes: read a wide variety of code data files

    Copyright (C) 2019 Christian W. Zuckschwerdt <zany@triq.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef INCLUDE_CODES_H_
#define INCLUDE_CODES_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define LINE_MAX 255
#define MSG_MAX 19
#define LIST_MAX 65536

struct data {
    uint8_t d[MSG_MAX];
    uint8_t chk;
    uint16_t chk16;
    uint16_t bit_len;
    char *comment;
};

struct data_list {
    struct data data[LIST_MAX];
    unsigned msg_len;
    unsigned list_len;
    char *header;
};

int parse_code(char const *text, struct data *data);

int read_codes(char const *filename, struct data *data, unsigned *msg_len, unsigned msg_max, unsigned list_max);

int sprint_code(char *dst, struct data *data, unsigned msg_len);

void print_codes(struct data *data, unsigned msg_len, unsigned list_len);

void free_codes(struct data *data, unsigned list_len);

#endif /* INCLUDE_CODES_H_ */
