#include <string.h>
#include <stdio.h>
#include "printer.h"

static const char BASE58_ALPHABET[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

int format_base58_address(const uint8_t pubkey[32], char *out, size_t out_len) {
    if (pubkey == NULL || out == NULL || out_len < 2) {
        return -1;
    }

    uint8_t tmp[32];
    memcpy(tmp, pubkey, 32);

    char encoded[45];
    int encoded_len = 0;

    while (encoded_len < (int) sizeof(encoded) - 1) {
        int remainder = 0;
        bool all_zero = true;
        for (int i = 0; i < 32; i++) {
            int acc = remainder * 256 + tmp[i];
            tmp[i] = (uint8_t) (acc / 58);
            remainder = acc % 58;
            if (tmp[i] != 0) {
                all_zero = false;
            }
        }
        encoded[encoded_len++] = BASE58_ALPHABET[remainder];
        if (all_zero) {
            break;
        }
    }

    int leading_zeros = 0;
    while (leading_zeros < 32 && pubkey[leading_zeros] == 0) {
        leading_zeros++;
    }

    size_t total_len = (size_t) leading_zeros + (size_t) encoded_len;
    if (total_len + 1 > out_len) {
        return -1;
    }

    size_t pos = 0;
    for (int i = 0; i < leading_zeros; i++) {
        out[pos++] = '1';
    }
    for (int i = encoded_len - 1; i >= 0; i--) {
        out[pos++] = encoded[i];
    }
    out[pos] = '\0';

    return (int) pos;
}

int format_xeris_amount(uint64_t lamports, char *out, size_t out_len) {
    if (out == NULL || out_len < 16) {
        return -1;
    }

    uint64_t whole = lamports / 1000000000ULL;
    uint64_t frac = lamports % 1000000000ULL;

    char buf[48];
    int len;

    if (frac == 0) {
        len = snprintf(buf, sizeof(buf), "%llu.0 XRS", (unsigned long long) whole);
    } else {
        len = snprintf(buf, sizeof(buf), "%llu.%09llu XRS",
                       (unsigned long long) whole,
                       (unsigned long long) frac);
        if (len <= 0) {
            return -1;
        }
        int end = len - 4;
        while (end > 0 && buf[end - 1] == '0') {
            end--;
        }
        memmove(buf + end, " XRS", 5);
        len = end + 4;
    }

    if (len <= 0 || (size_t) len >= out_len) {
        return -1;
    }
    memcpy(out, buf, (size_t) len + 1);
    return len;
}

int format_blockhash(const uint8_t hash[32], char *out, size_t out_len) {
    return format_base58_address(hash, out, out_len);
}

int format_tx_summary(const transaction_t *tx,
                      char *type_out, size_t type_len,
                      char *detail_out, size_t detail_len) {
    if (tx == NULL || type_out == NULL || detail_out == NULL) {
        return -1;
    }

    switch (tx->type) {
        case TX_TYPE_TRANSFER: {
            if (type_len < 9) return -1;
            memcpy(type_out, "Transfer", 9);
            return format_xeris_amount(tx->transfer_amount, detail_out, detail_len);
        }
        case TX_TYPE_STAKE: {
            if (type_len < 6) return -1;
            memcpy(type_out, "Stake", 6);
            snprintf(detail_out, detail_len, "Stake operation");
            return 0;
        }
        case TX_TYPE_VOTE: {
            if (type_len < 5) return -1;
            memcpy(type_out, "Vote", 5);
            snprintf(detail_out, detail_len, "Vote transaction");
            return 0;
        }
        default: {
            if (type_len < 8) return -1;
            memcpy(type_out, "Unknown", 8);
            snprintf(detail_out, detail_len, "%u instruction(s)", tx->num_instructions);
            return 0;
        }
    }
}
