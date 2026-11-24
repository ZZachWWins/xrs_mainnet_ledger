#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "os.h"
#include "ux.h"
#include "cx.h"

#include "constants.h"
#include "sw.h"
#include "transaction/types.h"

typedef struct {
    uint32_t path[MAX_BIP32_PATH_LEN];
    uint8_t path_len;
} bip32_path_t;

typedef enum {
    STATE_NONE,
    STATE_PARSED,
    STATE_APPROVED,
} command_state_e;

typedef struct {
    bip32_path_t bip32_path;
    uint8_t raw_public_key[32];
    char address[ADDRESS_BASE58_MAX_LEN + 1];
} pubkey_ctx_t;

typedef struct {
    bip32_path_t bip32_path;
    transaction_t transaction;
    uint8_t raw_tx[MAX_RAW_TX_LEN];
    size_t raw_tx_len;
    command_state_e state;
} transaction_ctx_t;

typedef struct {
    bip32_path_t bip32_path;
    uint8_t raw_msg[MAX_RAW_MSG_LEN];
    size_t raw_msg_len;
} message_ctx_t;

typedef struct {
    union {
        pubkey_ctx_t pubkey;
        transaction_ctx_t transaction;
        message_ctx_t message;
    };
} global_ctx_t;

typedef struct {
    bool blind_signing_enabled;
} settings_t;

extern global_ctx_t G_context;
extern settings_t N_settings_real;
#define N_settings (*(volatile settings_t *) PIC(&N_settings_real))
