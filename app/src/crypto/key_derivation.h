#pragma once

#include <stdint.h>
#include <stddef.h>
#include "../globals.h"

int crypto_derive_public_key(const bip32_path_t *path, uint8_t raw_pubkey[32]);

int crypto_sign_message(const bip32_path_t *path,
                        const uint8_t *msg,
                        size_t msg_len,
                        uint8_t sig[64]);
