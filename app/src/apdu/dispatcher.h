#pragma once

#include <stdint.h>
#include <stddef.h>
#include "../globals.h"

typedef struct {
    uint8_t cla;
    uint8_t ins;
    uint8_t p1;
    uint8_t p2;
    uint8_t lc;
    const uint8_t *data;
} command_t;

int apdu_dispatcher(const command_t *cmd);
