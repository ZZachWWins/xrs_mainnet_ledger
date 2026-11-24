#include <string.h>
#include "sign_tx.h"
#include "../globals.h"
#include "../constants.h"
#include "../sw.h"
#include "../transaction/parser.h"
#include "../ui/display.h"
#include "io.h"

static int read_bip32_path(const uint8_t *data, uint8_t data_len, bip32_path_t *path) {
    if (data_len < 1) {
        return -1;
    }
    path->path_len = data[0];
    if (path->path_len < 1 || path->path_len > MAX_BIP32_PATH_LEN) {
        return -1;
    }
    if (data_len < 1 + path->path_len * 4) {
        return -1;
    }
    for (uint8_t i = 0; i < path->path_len; i++) {
        path->path[i] = ((uint32_t) data[1 + i * 4] << 24) |
                         ((uint32_t) data[2 + i * 4] << 16) |
                         ((uint32_t) data[3 + i * 4] << 8) |
                         ((uint32_t) data[4 + i * 4]);
    }
    return 0;
}

int handler_sign_tx(const command_t *cmd) {
    transaction_ctx_t *ctx = &G_context.transaction;

    if (cmd->p1 == P1_FIRST) {
        explicit_bzero(ctx, sizeof(transaction_ctx_t));
        ctx->state = STATE_NONE;

        if (read_bip32_path(cmd->data, cmd->lc, &ctx->bip32_path) != 0) {
            return io_send_sw(SW_WRONG_DATA_LENGTH);
        }

        // Enforce Xeris derivation path
        if (ctx->bip32_path.path_len < 2 ||
            ctx->bip32_path.path[0] != 0x8000002C ||
            ctx->bip32_path.path[1] != 0x80002328) {
            return io_send_sw(SW_WRONG_P1P2);
        }

        // Forward any remaining payload bytes after the BIP32 path
        uint16_t path_bytes = 1 + (uint16_t) ctx->bip32_path.path_len * 4;
        if (cmd->lc > path_bytes) {
            uint16_t remaining = cmd->lc - path_bytes;
            if (remaining > MAX_RAW_TX_LEN) {
                return io_send_sw(SW_WRONG_TX_LENGTH);
            }
            memcpy(ctx->raw_tx, cmd->data + path_bytes, remaining);
            ctx->raw_tx_len = remaining;
        }

        return io_send_sw(SW_OK);
    }

    if (cmd->p1 != P1_MORE && cmd->p1 != P1_LAST) {
        return io_send_sw(SW_WRONG_P1P2);
    }

    if (ctx->raw_tx_len + cmd->lc > MAX_RAW_TX_LEN) {
        return io_send_sw(SW_WRONG_TX_LENGTH);
    }

    memcpy(ctx->raw_tx + ctx->raw_tx_len, cmd->data, cmd->lc);
    ctx->raw_tx_len += cmd->lc;

    if (cmd->p1 == P1_MORE) {
        return io_send_sw(SW_OK);
    }

    // P1_LAST — parse and display
    parser_status_e status = transaction_parse(ctx->raw_tx, ctx->raw_tx_len, &ctx->transaction);
    if (status != PARSING_OK) {
        return io_send_sw(SW_TX_PARSING_FAIL);
    }

    ctx->state = STATE_PARSED;

    if (ctx->transaction.type == TX_TYPE_UNKNOWN && !N_settings.blind_signing_enabled) {
        return io_send_sw(SW_DENY);
    }

    return ui_display_transaction();
}
