#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define MAX_TX_SIGNATURES    4
#define MAX_TX_ACCOUNT_KEYS  32
#define MAX_TX_INSTRUCTIONS  10
#define MAX_IX_ACCOUNTS      16
#define MAX_IX_DATA_LEN      256
#define PUBKEY_LEN           32
#define SIGNATURE_LEN        64
#define BLOCKHASH_LEN        32

typedef struct {
    uint8_t key[PUBKEY_LEN];
} pubkey_t;

typedef struct {
    uint8_t program_id_index;
    uint8_t account_indices[MAX_IX_ACCOUNTS];
    uint16_t num_accounts;
    uint8_t data[MAX_IX_DATA_LEN];
    uint16_t data_len;
} instruction_t;

typedef enum {
    TX_TYPE_UNKNOWN = 0,
    TX_TYPE_TRANSFER,
    TX_TYPE_STAKE,
    TX_TYPE_VOTE,
} tx_type_e;

typedef struct {
    uint8_t num_required_signatures;
    uint8_t num_readonly_signed;
    uint8_t num_readonly_unsigned;

    uint16_t num_account_keys;
    pubkey_t account_keys[MAX_TX_ACCOUNT_KEYS];

    uint8_t recent_blockhash[BLOCKHASH_LEN];

    uint16_t num_instructions;
    instruction_t instructions[MAX_TX_INSTRUCTIONS];

    uint16_t num_signatures;
    uint8_t signatures[MAX_TX_SIGNATURES][SIGNATURE_LEN];

    tx_type_e type;
    uint64_t transfer_amount;
    uint8_t sender_index;
    uint8_t recipient_index;

    const uint8_t *raw_message;
    size_t raw_message_len;
} transaction_t;
