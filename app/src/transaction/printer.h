#pragma once

#include <stdint.h>
#include <stddef.h>
#include "types.h"

int format_xeris_amount(uint64_t lamports, char *out, size_t out_len);

int format_base58_address(const uint8_t pubkey[32], char *out, size_t out_len);

int format_blockhash(const uint8_t hash[32], char *out, size_t out_len);

int format_tx_summary(const transaction_t *tx,
                      char *type_out, size_t type_len,
                      char *detail_out, size_t detail_len);
