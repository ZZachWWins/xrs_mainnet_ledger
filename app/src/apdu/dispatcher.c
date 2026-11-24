#include "dispatcher.h"
#include "get_version.h"
#include "get_pubkey.h"
#include "sign_tx.h"
#include "sign_message.h"
#include "../sw.h"
#include "../constants.h"
#include "io.h"

int apdu_dispatcher(const command_t *cmd) {
    if (cmd->cla != CLA_APP) {
        return io_send_sw(SW_CLA_NOT_SUPPORTED);
    }

    switch (cmd->ins) {
        case INS_GET_VERSION:
            return handler_get_version();
        case INS_GET_PUBKEY:
        case INS_GET_ADDRESS:
            return handler_get_pubkey(cmd);
        case INS_SIGN_TX:
            return handler_sign_tx(cmd);
        case INS_SIGN_MESSAGE:
            return handler_sign_message(cmd);
        default:
            return io_send_sw(SW_INS_NOT_SUPPORTED);
    }
}
