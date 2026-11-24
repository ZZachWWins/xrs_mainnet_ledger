#include <string.h>

#include "os.h"
#include "io.h"

#include "sign_message.h"
#include "../constants.h"
#include "../sw.h"
#include "../globals.h"
#include "../crypto/key_derivation.h"
#include "../ui/display.h"

static int parse_bip32_path(const uint8_t *data, uint8_t data_len,
                            bip32_path_t *out) {
    if (data_len < 1) {
        return -1;
    }

    out->path_len = data[0];
    if (out->path_len == 0 || out->path_len > MAX_BIP32_PATH_LEN) {
        return -1;
    }
    if (data_len < 1 + out->path_len * 4) {
        return -1;
    }

    for (uint8_t i = 0; i < out->path_len; i++) {
        out->path[i] = ((uint32_t) data[1 + i * 4] << 24) |
                        ((uint32_t) data[2 + i * 4] << 16) |
                        ((uint32_t) data[3 + i * 4] << 8) |
                        ((uint32_t) data[4 + i * 4]);
    }

    return 0;
}

int handler_sign_message(const command_t *cmd) {
    message_ctx_t *ctx = &G_context.message;

    if (cmd->p1 == P1_FIRST) {
        explicit_bzero(&G_context, sizeof(G_context));
        ctx->raw_msg_len = 0;

        if (cmd->lc < 1) {
            return io_send_sw(SW_WRONG_DATA_LENGTH);
        }

        uint16_t path_len_bytes = 1 + (uint16_t) cmd->data[0] * 4;
        if (cmd->lc < path_len_bytes) {
            return io_send_sw(SW_WRONG_DATA_LENGTH);
        }

        if (parse_bip32_path(cmd->data, cmd->lc, &ctx->bip32_path) != 0) {
            return io_send_sw(SW_WRONG_DATA_LENGTH);
        }

        uint16_t remaining = cmd->lc - path_len_bytes;
        if (remaining > 0) {
            if (remaining > MAX_RAW_MSG_LEN) {
                return io_send_sw(SW_WRONG_TX_LENGTH);
            }
            memcpy(ctx->raw_msg, cmd->data + path_len_bytes, remaining);
            ctx->raw_msg_len = remaining;
        }

        return io_send_sw(SW_OK);

    } else if (cmd->p1 == P1_MORE) {
        if (ctx->raw_msg_len + cmd->lc > MAX_RAW_MSG_LEN) {
            return io_send_sw(SW_WRONG_TX_LENGTH);
        }

        memcpy(ctx->raw_msg + ctx->raw_msg_len, cmd->data, cmd->lc);
        ctx->raw_msg_len += cmd->lc;

        return io_send_sw(SW_OK);

    } else if (cmd->p1 == P1_LAST) {
        if (cmd->lc > 0) {
            if (ctx->raw_msg_len + cmd->lc > MAX_RAW_MSG_LEN) {
                return io_send_sw(SW_WRONG_TX_LENGTH);
            }
            memcpy(ctx->raw_msg + ctx->raw_msg_len, cmd->data, cmd->lc);
            ctx->raw_msg_len += cmd->lc;
        }

        if (ctx->raw_msg_len == 0) {
            return io_send_sw(SW_WRONG_DATA_LENGTH);
        }

        return ui_display_message();

    } else {
        return io_send_sw(SW_WRONG_P1P2);
    }
}
