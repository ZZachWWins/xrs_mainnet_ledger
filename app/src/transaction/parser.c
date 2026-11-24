#include <string.h>
#include "parser.h"

static const uint8_t SYSTEM_PROGRAM_ID[PUBKEY_LEN] = {0};
#define SYSTEM_IX_TRANSFER 2

int read_compact_u16(const uint8_t *buf, size_t len, uint16_t *value, size_t *bytes_read) {
    if (len == 0 || buf == NULL || value == NULL || bytes_read == NULL) {
        return -1;
    }

    uint8_t b0 = buf[0];
    if (b0 < 0x80) {
        *value = b0;
        *bytes_read = 1;
        return 0;
    }

    if (len < 2) {
        return -1;
    }

    uint8_t b1 = buf[1];
    if (b1 < 0x80) {
        *value = (uint16_t) ((b0 & 0x7F) | (b1 << 7));
        *bytes_read = 2;
        return 0;
    }

    if (len < 3) {
        return -1;
    }

    uint8_t b2 = buf[2];
    if (b2 > 0x03) {
        return -1;
    }
    *value = (uint16_t) ((b0 & 0x7F) | ((b1 & 0x7F) << 7) | (b2 << 14));
    *bytes_read = 3;
    return 0;
}

static parser_status_e parse_message(const uint8_t *buf, size_t len, transaction_t *tx) {
    size_t offset = 0;

    if (len < 3) {
        return PARSING_INVALID_HEADER;
    }
    tx->num_required_signatures = buf[offset++];
    tx->num_readonly_signed = buf[offset++];
    tx->num_readonly_unsigned = buf[offset++];

    if (tx->num_required_signatures == 0) {
        return PARSING_INVALID_HEADER;
    }

    uint16_t num_keys;
    size_t compact_len;
    if (read_compact_u16(buf + offset, len - offset, &num_keys, &compact_len) != 0) {
        return PARSING_INVALID_COMPACT_U16;
    }
    offset += compact_len;

    if (num_keys > MAX_TX_ACCOUNT_KEYS) {
        return PARSING_TOO_MANY_ACCOUNTS;
    }
    if (num_keys == 0) {
        return PARSING_INVALID_HEADER;
    }
    if ((uint16_t) tx->num_required_signatures > num_keys) {
        return PARSING_INVALID_HEADER;
    }
    tx->num_account_keys = num_keys;

    size_t keys_size = (size_t) num_keys * PUBKEY_LEN;
    if (offset + keys_size > len) {
        return PARSING_UNEXPECTED_BUFFER_END;
    }
    for (uint16_t i = 0; i < num_keys; i++) {
        memcpy(tx->account_keys[i].key, buf + offset, PUBKEY_LEN);
        offset += PUBKEY_LEN;
    }

    if (offset + BLOCKHASH_LEN > len) {
        return PARSING_UNEXPECTED_BUFFER_END;
    }
    memcpy(tx->recent_blockhash, buf + offset, BLOCKHASH_LEN);
    offset += BLOCKHASH_LEN;

    uint16_t num_ix;
    if (read_compact_u16(buf + offset, len - offset, &num_ix, &compact_len) != 0) {
        return PARSING_INVALID_COMPACT_U16;
    }
    offset += compact_len;

    if (num_ix > MAX_TX_INSTRUCTIONS) {
        return PARSING_TOO_MANY_INSTRUCTIONS;
    }
    tx->num_instructions = num_ix;

    for (uint16_t i = 0; i < num_ix; i++) {
        instruction_t *ix = &tx->instructions[i];

        if (offset >= len) {
            return PARSING_UNEXPECTED_BUFFER_END;
        }
        ix->program_id_index = buf[offset++];
        if (ix->program_id_index >= tx->num_account_keys) {
            return PARSING_INVALID_ACCOUNT_INDEX;
        }

        uint16_t num_accts;
        if (read_compact_u16(buf + offset, len - offset, &num_accts, &compact_len) != 0) {
            return PARSING_INVALID_COMPACT_U16;
        }
        offset += compact_len;

        if (num_accts > MAX_IX_ACCOUNTS) {
            return PARSING_TOO_MANY_ACCOUNTS;
        }
        ix->num_accounts = num_accts;

        if (offset + num_accts > len) {
            return PARSING_UNEXPECTED_BUFFER_END;
        }
        for (uint16_t j = 0; j < num_accts; j++) {
            ix->account_indices[j] = buf[offset++];
            if (ix->account_indices[j] >= tx->num_account_keys) {
                return PARSING_INVALID_ACCOUNT_INDEX;
            }
        }

        uint16_t data_len;
        if (read_compact_u16(buf + offset, len - offset, &data_len, &compact_len) != 0) {
            return PARSING_INVALID_COMPACT_U16;
        }
        offset += compact_len;

        if (data_len > MAX_IX_DATA_LEN) {
            return PARSING_INVALID_LENGTH;
        }
        if (offset + data_len > len) {
            return PARSING_UNEXPECTED_BUFFER_END;
        }
        ix->data_len = data_len;
        memcpy(ix->data, buf + offset, data_len);
        offset += data_len;
    }

    return PARSING_OK;
}

static void detect_transfer(transaction_t *tx) {
    tx->type = TX_TYPE_UNKNOWN;

    if (tx->num_instructions != 1) {
        return;
    }

    const instruction_t *ix = &tx->instructions[0];

    if (ix->program_id_index >= tx->num_account_keys) {
        return;
    }
    if (memcmp(tx->account_keys[ix->program_id_index].key, SYSTEM_PROGRAM_ID, PUBKEY_LEN) != 0) {
        return;
    }

    if (ix->data_len != 12) {
        return;
    }

    uint32_t ix_type = (uint32_t) ix->data[0] |
                       ((uint32_t) ix->data[1] << 8) |
                       ((uint32_t) ix->data[2] << 16) |
                       ((uint32_t) ix->data[3] << 24);

    if (ix_type != SYSTEM_IX_TRANSFER) {
        return;
    }

    if (ix->num_accounts != 2) {
        return;
    }

    tx->type = TX_TYPE_TRANSFER;
    tx->sender_index = ix->account_indices[0];
    tx->recipient_index = ix->account_indices[1];
    tx->transfer_amount = (uint64_t) ix->data[4] |
                          ((uint64_t) ix->data[5] << 8) |
                          ((uint64_t) ix->data[6] << 16) |
                          ((uint64_t) ix->data[7] << 24) |
                          ((uint64_t) ix->data[8] << 32) |
                          ((uint64_t) ix->data[9] << 40) |
                          ((uint64_t) ix->data[10] << 48) |
                          ((uint64_t) ix->data[11] << 56);
}

parser_status_e transaction_parse(const uint8_t *raw, size_t len, transaction_t *tx) {
    if (raw == NULL || tx == NULL || len == 0) {
        return PARSING_INVALID_LENGTH;
    }

    memset(tx, 0, sizeof(transaction_t));

    size_t offset = 0;

    uint16_t num_sigs;
    size_t compact_len;
    if (read_compact_u16(raw, len, &num_sigs, &compact_len) != 0) {
        return PARSING_INVALID_COMPACT_U16;
    }
    offset += compact_len;

    if (num_sigs > MAX_TX_SIGNATURES) {
        return PARSING_TOO_MANY_SIGNATURES;
    }
    tx->num_signatures = num_sigs;

    size_t sigs_size = (size_t) num_sigs * SIGNATURE_LEN;
    if (offset + sigs_size > len) {
        return PARSING_UNEXPECTED_BUFFER_END;
    }
    for (uint16_t i = 0; i < num_sigs; i++) {
        memcpy(tx->signatures[i], raw + offset, SIGNATURE_LEN);
        offset += SIGNATURE_LEN;
    }

    tx->raw_message = raw + offset;
    tx->raw_message_len = len - offset;

    if (tx->raw_message_len == 0) {
        return PARSING_UNEXPECTED_BUFFER_END;
    }

    parser_status_e status = parse_message(tx->raw_message, tx->raw_message_len, tx);
    if (status != PARSING_OK) {
        return status;
    }

    detect_transfer(tx);

    return PARSING_OK;
}
