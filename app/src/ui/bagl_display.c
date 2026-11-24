#ifdef HAVE_BAGL

#include <string.h>
#include "bagl_display.h"
#include "display.h"
#include "../globals.h"
#include "../constants.h"

extern const char *ui_get_type(void);
extern const char *ui_get_amount(void);
extern const char *ui_get_sender(void);
extern const char *ui_get_recipient(void);
extern const char *ui_get_blockhash(void);
extern const char *ui_get_address(void);

UX_STEP_NOCB(ux_addr_review_step,
             pnn,
             {&C_icon_eye,
              "Verify",
              "address"});
UX_STEP_NOCB(ux_addr_display_step,
             bnnn_paging,
             {"Address",
              ui_get_address()});
UX_STEP_CB(ux_addr_approve_step,
            pb,
            ui_action_confirm(true),
            {&C_icon_validate_14,
             "Approve"});
UX_STEP_CB(ux_addr_reject_step,
            pb,
            ui_action_confirm(false),
            {&C_icon_crossmark,
             "Reject"});

UX_FLOW(ux_addr_flow,
        &ux_addr_review_step,
        &ux_addr_display_step,
        &ux_addr_approve_step,
        &ux_addr_reject_step);

int bagl_display_address(void) {
    ux_flow_init(0, ux_addr_flow, NULL);
    return 0;
}

UX_STEP_NOCB(ux_tx_review_step,
             pnn,
             {&C_icon_eye,
              "Review",
              "transaction"});
UX_STEP_NOCB(ux_tx_type_step,
             bnnn_paging,
             {"Type",
              ui_get_type()});
UX_STEP_NOCB(ux_tx_amount_step,
             bnnn_paging,
             {"Amount",
              ui_get_amount()});
UX_STEP_NOCB(ux_tx_sender_step,
             bnnn_paging,
             {"Sender",
              ui_get_sender()});
UX_STEP_NOCB(ux_tx_recipient_step,
             bnnn_paging,
             {"Recipient",
              ui_get_recipient()});
UX_STEP_NOCB(ux_tx_blockhash_step,
             bnnn_paging,
             {"Blockhash",
              ui_get_blockhash()});
UX_STEP_CB(ux_tx_approve_step,
            pb,
            ui_action_confirm(true),
            {&C_icon_validate_14,
             "Approve"});
UX_STEP_CB(ux_tx_reject_step,
            pb,
            ui_action_confirm(false),
            {&C_icon_crossmark,
             "Reject"});

UX_FLOW(ux_tx_flow,
        &ux_tx_review_step,
        &ux_tx_type_step,
        &ux_tx_amount_step,
        &ux_tx_sender_step,
        &ux_tx_recipient_step,
        &ux_tx_blockhash_step,
        &ux_tx_approve_step,
        &ux_tx_reject_step);

int bagl_display_transaction(void) {
    ux_flow_init(0, ux_tx_flow, NULL);
    return 0;
}

UX_STEP_NOCB(ux_msg_review_step,
             pnn,
             {&C_icon_eye,
              "Review",
              "message"});
UX_STEP_NOCB(ux_msg_info_step,
             bnnn_paging,
             {"Message",
              "Sign off-chain message"});
UX_STEP_CB(ux_msg_approve_step,
            pb,
            ui_action_confirm(true),
            {&C_icon_validate_14,
             "Approve"});
UX_STEP_CB(ux_msg_reject_step,
            pb,
            ui_action_confirm(false),
            {&C_icon_crossmark,
             "Reject"});

UX_FLOW(ux_msg_flow,
        &ux_msg_review_step,
        &ux_msg_info_step,
        &ux_msg_approve_step,
        &ux_msg_reject_step);

int bagl_display_message(void) {
    ux_flow_init(0, ux_msg_flow, NULL);
    return 0;
}

UX_STEP_NOCB(ux_idle_step,
             nn,
             {"Xeris", "is ready"});
UX_STEP_NOCB(ux_version_step,
             bn,
             {"Version", APPVERSION});
UX_STEP_CB(ux_quit_step,
            pb,
            os_sched_exit(-1),
            {&C_icon_dashboard_x,
             "Quit"});

UX_FLOW(ux_idle_flow,
        &ux_idle_step,
        &ux_version_step,
        &ux_quit_step);

void bagl_menu_main(void) {
    if (G_ux.stack_count == 0) {
        ux_stack_push();
    }
    ux_flow_init(0, ux_idle_flow, NULL);
}

#endif
