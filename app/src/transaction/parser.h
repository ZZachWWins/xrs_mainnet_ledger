#pragma once

#include <stdint.h>
#include <stddef.h>
#include "types.h"

typedef enum {
    PARSING_OK = 0,
    PARSING_INVALID_LENGTH,
    PARSING_UNEXPECTED_BUFFER_END,
    PARSING_INVALID_INSTRUCTION,
    PARSING_INVALID_ACCOUNT_INDEX,
    PARSING_TOO_MANY_SIGNATURES,
    PARSING_TOO_MANY_ACCOUNTS,
    PARSING_TOO_MANY_INSTRUCTIONS,
    PARSING_INVALID_COMPACT_U16,
    PARSING_INVALID_HEADER,
} parser_status_e;

int read_compact_u16(const uint8_t *buf, size_t len, uint16_t *value, size_t *bytes_read);

parser_status_e transaction_parse(const uint8_t *raw, size_t len, transaction_t *tx);
