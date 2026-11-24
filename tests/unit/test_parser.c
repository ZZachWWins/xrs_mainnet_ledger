#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
 * Standalone unit tests for the transaction parser.
 * These pull in parser logic via source includes so they can run on the host
 * without the Ledger SDK.
 */

/* Minimal type stubs matching app types */
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

/* -- Compact-u16 decoder (mirrors app/src/transaction/parser.c) -- */

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

/* -- Transaction parser (mirrors app/src/transaction/parser.c) -- */

static int transaction_parse(const uint8_t *buf, size_t buf_len, transaction_t *tx) {
    size_t offset = 0;
    size_t read;
    uint16_t val;

    memset(tx, 0, sizeof(transaction_t));

    /* Header: 3 bytes */
    if (buf_len < 3) return -1;
    tx->num_required_signatures = buf[offset++];
    tx->num_readonly_signed = buf[offset++];
    tx->num_readonly_unsigned = buf[offset++];

    if (tx->num_required_signatures == 0) return -1;

    /* Account keys */
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

    /* Recent blockhash */
    if (offset + BLOCKHASH_LEN > buf_len) return -1;
    memcpy(tx->blockhash, buf + offset, BLOCKHASH_LEN);
    offset += BLOCKHASH_LEN;

    /* Instructions */
    if (decode_compact_u16(buf + offset, buf_len - offset, &val, &read) != 0) return -1;
    offset += read;
    tx->num_instructions = (uint8_t)val;

    if (tx->num_instructions > MAX_TX_INSTRUCTIONS) return -1;
    if (tx->num_instructions == 0) return -1;

    for (int i = 0; i < tx->num_instructions; i++) {
        instruction_t *ix = &tx->instructions[i];

        /* Program ID index */
        if (offset >= buf_len) return -1;
        ix->program_id_index = buf[offset++];
        if (ix->program_id_index >= tx->num_accounts) return -1;

        /* Account indices */
        if (decode_compact_u16(buf + offset, buf_len - offset, &val, &read) != 0) return -1;
        offset += read;
        ix->num_account_indices = (uint8_t)val;
        if (ix->num_account_indices > MAX_TX_ACCOUNTS) return -1;

        for (int j = 0; j < ix->num_account_indices; j++) {
            if (offset >= buf_len) return -1;
            ix->account_indices[j] = buf[offset++];
            if (ix->account_indices[j] >= tx->num_accounts) return -1;
        }

        /* Instruction data */
        if (decode_compact_u16(buf + offset, buf_len - offset, &val, &read) != 0) return -1;
        offset += read;
        ix->data_len = val;
        if (offset + ix->data_len > buf_len) return -1;
        ix->data = buf + offset;
        offset += ix->data_len;
    }

    return 0;
}

/* ================= Tests ================= */

static int tests_run = 0;
static int tests_passed = 0;

#define RUN_TEST(fn) do { \
    tests_run++; \
    printf("  %s ... ", #fn); \
    fn(); \
    tests_passed++; \
    printf("PASS\n"); \
} while(0)

static void test_compact_u16_single_byte(void) {
    uint8_t buf[] = {0x05};
    uint16_t val;
    size_t read;
    assert(decode_compact_u16(buf, sizeof(buf), &val, &read) == 0);
    assert(val == 5);
    assert(read == 1);
}

static void test_compact_u16_two_bytes(void) {
    uint8_t buf[] = {0x80, 0x01};
    uint16_t val;
    size_t read;
    assert(decode_compact_u16(buf, sizeof(buf), &val, &read) == 0);
    assert(val == 128);
    assert(read == 2);
}

static void test_compact_u16_three_bytes(void) {
    uint8_t buf[] = {0x80, 0x80, 0x01};
    uint16_t val;
    size_t read;
    assert(decode_compact_u16(buf, sizeof(buf), &val, &read) == 0);
    assert(val == 16384);
    assert(read == 3);
}

static void test_compact_u16_zero(void) {
    uint8_t buf[] = {0x00};
    uint16_t val;
    size_t read;
    assert(decode_compact_u16(buf, sizeof(buf), &val, &read) == 0);
    assert(val == 0);
    assert(read == 1);
}

static void test_compact_u16_max_single(void) {
    uint8_t buf[] = {0x7F};
    uint16_t val;
    size_t read;
    assert(decode_compact_u16(buf, sizeof(buf), &val, &read) == 0);
    assert(val == 127);
    assert(read == 1);
}

static void test_compact_u16_empty_buffer(void) {
    uint16_t val;
    size_t read;
    assert(decode_compact_u16(NULL, 0, &val, &read) == -1);
}

static void test_compact_u16_truncated_two(void) {
    uint8_t buf[] = {0x80};
    uint16_t val;
    size_t read;
    assert(decode_compact_u16(buf, 1, &val, &read) == -1);
}

static void test_parse_simple_transfer(void) {
    /*
     * Minimal transfer: 3 accounts (from, to, system), 1 instruction
     * Header: num_sigs=1, ro_signed=0, ro_unsigned=1
     */
    uint8_t msg[256];
    size_t offset = 0;

    /* Header */
    msg[offset++] = 1;  /* num_required_signatures */
    msg[offset++] = 0;  /* num_readonly_signed */
    msg[offset++] = 1;  /* num_readonly_unsigned */

    /* Num accounts: 3 */
    msg[offset++] = 3;

    /* Account 0 (from) */
    memset(msg + offset, 0xAA, 32);
    offset += 32;

    /* Account 1 (to) */
    memset(msg + offset, 0xBB, 32);
    offset += 32;

    /* Account 2 (system program = all zeros) */
    memset(msg + offset, 0x00, 32);
    offset += 32;

    /* Recent blockhash */
    memset(msg + offset, 0xCC, 32);
    offset += 32;

    /* Num instructions: 1 */
    msg[offset++] = 1;

    /* Instruction: program_id_index = 2 (system) */
    msg[offset++] = 2;

    /* Num account indices: 2 */
    msg[offset++] = 2;
    msg[offset++] = 0;  /* from */
    msg[offset++] = 1;  /* to */

    /* Instruction data: transfer type (4 bytes) + amount (8 bytes) = 12 bytes */
    msg[offset++] = 12;

    /* Transfer instruction type = 2 */
    msg[offset++] = 2;
    msg[offset++] = 0;
    msg[offset++] = 0;
    msg[offset++] = 0;

    /* Amount: 1 XRS = 1,000,000,000 lamports = 0x3B9ACA00 */
    msg[offset++] = 0x00;
    msg[offset++] = 0xCA;
    msg[offset++] = 0x9A;
    msg[offset++] = 0x3B;
    msg[offset++] = 0x00;
    msg[offset++] = 0x00;
    msg[offset++] = 0x00;
    msg[offset++] = 0x00;

    transaction_t tx;
    int rc = transaction_parse(msg, offset, &tx);

    assert(rc == 0);
    assert(tx.num_required_signatures == 1);
    assert(tx.num_readonly_signed == 0);
    assert(tx.num_readonly_unsigned == 1);
    assert(tx.num_accounts == 3);
    assert(tx.num_instructions == 1);
    assert(tx.instructions[0].program_id_index == 2);
    assert(tx.instructions[0].num_account_indices == 2);
    assert(tx.instructions[0].data_len == 12);
}

static void test_parse_empty_message(void) {
    transaction_t tx;
    assert(transaction_parse(NULL, 0, &tx) == -1);
}

static void test_parse_header_only(void) {
    uint8_t msg[] = {1, 0, 0};
    transaction_t tx;
    assert(transaction_parse(msg, sizeof(msg), &tx) == -1);
}

static void test_parse_zero_signatures(void) {
    uint8_t msg[] = {0, 0, 0, 1};
    transaction_t tx;
    assert(transaction_parse(msg, sizeof(msg), &tx) == -1);
}

static void test_parse_too_many_accounts(void) {
    uint8_t msg[4];
    msg[0] = 1;
    msg[1] = 0;
    msg[2] = 0;
    msg[3] = MAX_TX_ACCOUNTS + 1;

    transaction_t tx;
    assert(transaction_parse(msg, sizeof(msg), &tx) == -1);
}

static void test_parse_account_count_less_than_sigs(void) {
    uint8_t msg[4];
    msg[0] = 5;  /* num_required_signatures */
    msg[1] = 0;
    msg[2] = 0;
    msg[3] = 3;  /* num_accounts < num_required_signatures */

    transaction_t tx;
    assert(transaction_parse(msg, sizeof(msg), &tx) == -1);
}

static void test_parse_truncated_accounts(void) {
    uint8_t msg[5];
    msg[0] = 1;
    msg[1] = 0;
    msg[2] = 0;
    msg[3] = 2;
    msg[4] = 0xAA;  /* Only 1 byte of a 32-byte pubkey */

    transaction_t tx;
    assert(transaction_parse(msg, sizeof(msg), &tx) == -1);
}

static void test_parse_instruction_oob_program_id(void) {
    uint8_t msg[256];
    size_t offset = 0;

    msg[offset++] = 1;
    msg[offset++] = 0;
    msg[offset++] = 0;
    msg[offset++] = 1;
    memset(msg + offset, 0xAA, 32);
    offset += 32;
    memset(msg + offset, 0xCC, 32);
    offset += 32;
    msg[offset++] = 1;
    msg[offset++] = 5;  /* program_id_index = 5, but only 1 account */

    transaction_t tx;
    assert(transaction_parse(msg, offset, &tx) == -1);
}

int main(void) {
    printf("Running Xeris transaction parser unit tests\n");

    RUN_TEST(test_compact_u16_single_byte);
    RUN_TEST(test_compact_u16_two_bytes);
    RUN_TEST(test_compact_u16_three_bytes);
    RUN_TEST(test_compact_u16_zero);
    RUN_TEST(test_compact_u16_max_single);
    RUN_TEST(test_compact_u16_empty_buffer);
    RUN_TEST(test_compact_u16_truncated_two);
    RUN_TEST(test_parse_simple_transfer);
    RUN_TEST(test_parse_empty_message);
    RUN_TEST(test_parse_header_only);
    RUN_TEST(test_parse_zero_signatures);
    RUN_TEST(test_parse_too_many_accounts);
    RUN_TEST(test_parse_account_count_less_than_sigs);
    RUN_TEST(test_parse_truncated_accounts);
    RUN_TEST(test_parse_instruction_oob_program_id);

    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return tests_run - tests_passed;
}
