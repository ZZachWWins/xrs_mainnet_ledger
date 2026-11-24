#include <stdint.h>
#include <stddef.h>
#include <string.h>

/*
 * Fuzzing harness for the Xeris transaction parser.
 *
 * Build with AFL:
 *   afl-gcc -g -fsanitize=address -o fuzz_parser fuzz_parser.c
 *   mkdir -p corpus && echo -ne '\x01\x00\x00\x01' > corpus/seed
 *   afl-fuzz -i corpus -o findings -- ./fuzz_parser
 *
 * Build with libFuzzer:
 *   clang -g -fsanitize=fuzzer,address -o fuzz_parser fuzz_parser.c
 *   mkdir -p corpus && echo -ne '\x01\x00\x00\x01' > corpus/seed
 *   ./fuzz_parser corpus/
 */

#define MAX_TX_ACCOUNTS    35
#define MAX_TX_INSTRUCTIONS 6
#define PUBKEY_LEN         32
#define BLOCKHASH_LEN      32

typedef struct {
    uint8_t data[PUBKEY_LEN];
} pubkey_t;

typedef struct {
    uint8_t program_id_index;
    uint8_t account_indices[MAX_TX_ACCOUNTS];
    uint8_t num_account_indices;
    const uint8_t *data;
    uint16_t data_len;
} instruction_t;

typedef struct {
    uint8_t num_required_signatures;
    uint8_t num_readonly_signed;
    uint8_t num_readonly_unsigned;
    uint8_t num_accounts;
    pubkey_t accounts[MAX_TX_ACCOUNTS];
    uint8_t blockhash[BLOCKHASH_LEN];
    uint8_t num_instructions;
    instruction_t instructions[MAX_TX_INSTRUCTIONS];
} transaction_t;

static int decode_compact_u16(const uint8_t *buf, size_t buf_len, uint16_t *out, size_t *bytes_read) {
    if (buf_len == 0) return -1;
    if (buf[0] < 0x80) {
        *out = buf[0];
        *bytes_read = 1;
        return 0;
    }
    if (buf_len < 2) return -1;
    if (buf[1] < 0x80) {
        *out = (uint16_t)((buf[0] & 0x7F) | ((uint16_t)buf[1] << 7));
        *bytes_read = 2;
        return 0;
    }
    if (buf_len < 3) return -1;
    *out = (uint16_t)((buf[0] & 0x7F) | ((uint16_t)(buf[1] & 0x7F) << 7) | ((uint16_t)(buf[2] & 0x03) << 14));
    *bytes_read = 3;
    return 0;
}

static int transaction_parse(const uint8_t *buf, size_t buf_len, transaction_t *tx) {
    size_t offset = 0;
    size_t read;
    uint16_t val;

    memset(tx, 0, sizeof(transaction_t));

    if (buf_len < 3) return -1;
    tx->num_required_signatures = buf[offset++];
    tx->num_readonly_signed = buf[offset++];
    tx->num_readonly_unsigned = buf[offset++];
    if (tx->num_required_signatures == 0) return -1;

    if (decode_compact_u16(buf + offset, buf_len - offset, &val, &read) != 0) return -1;
    offset += read;
    tx->num_accounts = (uint8_t)val;
    if (tx->num_accounts > MAX_TX_ACCOUNTS) return -1;
    if (tx->num_accounts < tx->num_required_signatures) return -1;

    for (int i = 0; i < tx->num_accounts; i++) {
        if (offset + PUBKEY_LEN > buf_len) return -1;
        memcpy(tx->accounts[i].data, buf + offset, PUBKEY_LEN);
        offset += PUBKEY_LEN;
    }

    if (offset + BLOCKHASH_LEN > buf_len) return -1;
    memcpy(tx->blockhash, buf + offset, BLOCKHASH_LEN);
    offset += BLOCKHASH_LEN;

    if (decode_compact_u16(buf + offset, buf_len - offset, &val, &read) != 0) return -1;
    offset += read;
    tx->num_instructions = (uint8_t)val;
    if (tx->num_instructions > MAX_TX_INSTRUCTIONS) return -1;
    if (tx->num_instructions == 0) return -1;

    for (int i = 0; i < tx->num_instructions; i++) {
        instruction_t *ix = &tx->instructions[i];
        if (offset >= buf_len) return -1;
        ix->program_id_index = buf[offset++];
        if (ix->program_id_index >= tx->num_accounts) return -1;

        if (decode_compact_u16(buf + offset, buf_len - offset, &val, &read) != 0) return -1;
        offset += read;
        ix->num_account_indices = (uint8_t)val;
        if (ix->num_account_indices > MAX_TX_ACCOUNTS) return -1;
        for (int j = 0; j < ix->num_account_indices; j++) {
            if (offset >= buf_len) return -1;
            ix->account_indices[j] = buf[offset++];
            if (ix->account_indices[j] >= tx->num_accounts) return -1;
        }

        if (decode_compact_u16(buf + offset, buf_len - offset, &val, &read) != 0) return -1;
        offset += read;
        ix->data_len = val;
        if (offset + ix->data_len > buf_len) return -1;
        ix->data = buf + offset;
        offset += ix->data_len;
    }

    return 0;
}

#ifdef __AFL_FUZZ_TESTCASE_LEN

__AFL_FUZZ_INIT();

int main(void) {
    __AFL_INIT();
    unsigned char *buf = __AFL_FUZZ_TESTCASE_BUF;
    while (__AFL_LOOP(10000)) {
        int len = __AFL_FUZZ_TESTCASE_LEN;
        transaction_t tx;
        transaction_parse(buf, (size_t)len, &tx);
    }
    return 0;
}

#elif defined(__LIBFUZZER__)

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    transaction_t tx;
    transaction_parse(data, size, &tx);
    return 0;
}

#else
/* libFuzzer default entry */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    transaction_t tx;
    transaction_parse(data, size, &tx);
    return 0;
}
#endif
