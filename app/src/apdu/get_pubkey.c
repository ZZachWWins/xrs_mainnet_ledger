#include <string.h>
#include "get_pubkey.h"
#include "../globals.h"
#include "../constants.h"
#include "../sw.h"
#include "../crypto/key_derivation.h"
#include "../transaction/printer.h"
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

int handler_get_pubkey(const command_t *cmd) {
    pubkey_ctx_t *ctx = &G_context.pubkey;

    if (read_bip32_path(cmd->data, cmd->lc, &ctx->bip32_path) != 0) {
        return io_send_sw(SW_WRONG_DATA_LENGTH);
    }

    // Enforce Xeris derivation path: m/44'/9000'/...
    if (ctx->bip32_path.path_len < 2 ||
        ctx->bip32_path.path[0] != 0x8000002C ||  // 44'
        ctx->bip32_path.path[1] != 0x80002328) {   // 9000'
        return io_send_sw(SW_WRONG_P1P2);
    }

    if (crypto_derive_public_key(&ctx->bip32_path, ctx->raw_public_key) != 0) {
        return io_send_sw(SW_INTERNAL_ERROR);
    }

    if (format_base58_address(ctx->raw_public_key, ctx->address, sizeof(ctx->address)) < 0) {
        return io_send_sw(SW_DISPLAY_ADDRESS_FAIL);
    }

    if (cmd->p1 == P1_CONFIRM) {
        return ui_display_address();
    }

    return io_send_response_pointer(ctx->raw_public_key, 32, SW_OK);
}
