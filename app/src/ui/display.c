#include <string.h>
#include "display.h"
#include "../globals.h"
#include "../transaction/printer.h"
#include "io.h"

static char g_type[32];
static char g_amount[48];
static char g_sender[ADDRESS_BASE58_MAX_LEN + 1];
static char g_recipient[ADDRESS_BASE58_MAX_LEN + 1];
static char g_blockhash[ADDRESS_BASE58_MAX_LEN + 1];
static char g_address[ADDRESS_BASE58_MAX_LEN + 1];

const char *ui_get_type(void)      { return g_type; }
const char *ui_get_amount(void)    { return g_amount; }
const char *ui_get_sender(void)    { return g_sender; }
const char *ui_get_recipient(void) { return g_recipient; }
const char *ui_get_blockhash(void) { return g_blockhash; }
const char *ui_get_address(void)   { return g_address; }

int ui_display_address(void) {
    memcpy(g_address,
           G_context.pubkey.address,
           sizeof(G_context.pubkey.address));

#ifdef HAVE_NBGL
    extern int nbgl_display_address(void);
    return nbgl_display_address();
#elif defined(HAVE_BAGL)
    extern int bagl_display_address(void);
    return bagl_display_address();
#else
    return -1;
#endif
}

int ui_display_transaction(void) {
    transaction_t *tx = &G_context.transaction.transaction;

    format_tx_summary(tx, g_type, sizeof(g_type), g_amount, sizeof(g_amount));

    if (tx->type == TX_TYPE_TRANSFER &&
        tx->sender_index < tx->num_account_keys &&
        tx->recipient_index < tx->num_account_keys) {
        format_base58_address(
            tx->account_keys[tx->sender_index].key,
            g_sender, sizeof(g_sender));
        format_base58_address(
            tx->account_keys[tx->recipient_index].key,
            g_recipient, sizeof(g_recipient));
    } else {
        memcpy(g_sender, "N/A", 4);
        memcpy(g_recipient, "N/A", 4);
    }

    format_blockhash(tx->recent_blockhash, g_blockhash, sizeof(g_blockhash));

#ifdef HAVE_NBGL
    extern int nbgl_display_transaction(void);
    return nbgl_display_transaction();
#elif defined(HAVE_BAGL)
    extern int bagl_display_transaction(void);
    return bagl_display_transaction();
#else
    return -1;
#endif
}

int ui_display_message(void) {
#ifdef HAVE_NBGL
    extern int nbgl_display_message(void);
    return nbgl_display_message();
#elif defined(HAVE_BAGL)
    extern int bagl_display_message(void);
    return bagl_display_message();
#else
    return -1;
#endif
}

void ui_action_confirm_tx(bool approved) {
    if (approved) {
        uint8_t sig[64] = {0};

        if (G_context.transaction.state != STATE_PARSED) {
            io_send_sw(SW_BAD_STATE);
            ui_menu_main();
            return;
        }

        int rc = crypto_sign_message(
            &G_context.transaction.bip32_path,
            G_context.transaction.transaction.raw_message,
            G_context.transaction.transaction.raw_message_len,
            sig);

        if (rc != 0) {
            io_send_sw(SW_SIGNATURE_FAIL);
        } else {
            io_send_response_pointer(sig, 64, SW_OK);
        }
    } else {
        io_send_sw(SW_DENY);
    }

    ui_menu_main();
}

void ui_action_confirm_msg(bool approved) {
    if (approved) {
        uint8_t sig[64] = {0};
        message_ctx_t *ctx = &G_context.message;

        int rc = crypto_sign_message(
            &ctx->bip32_path,
            ctx->raw_msg,
            ctx->raw_msg_len,
            sig);

        if (rc != 0) {
            io_send_sw(SW_SIGNATURE_FAIL);
        } else {
            io_send_response_pointer(sig, 64, SW_OK);
        }
    } else {
        io_send_sw(SW_DENY);
    }

    ui_menu_main();
}

void ui_action_confirm(bool approved) {
    ui_action_confirm_tx(approved);
}
