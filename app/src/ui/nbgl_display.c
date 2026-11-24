#ifdef HAVE_NBGL

#include <string.h>
#include "nbgl_display.h"
#include "display.h"
#include "../globals.h"
#include "../constants.h"
#include "nbgl_use_case.h"

extern const char *ui_get_type(void);
extern const char *ui_get_amount(void);
extern const char *ui_get_sender(void);
extern const char *ui_get_recipient(void);
extern const char *ui_get_blockhash(void);
extern const char *ui_get_address(void);

static void confirm_callback(bool confirmed) {
    ui_action_confirm(confirmed);
}

static void address_verification_callback(bool confirmed) {
    if (confirmed) {
        nbgl_useCaseReviewStatus(STATUS_TYPE_ADDRESS_VERIFIED, ui_menu_main);
    } else {
        nbgl_useCaseReviewStatus(STATUS_TYPE_ADDRESS_REJECTED, ui_menu_main);
    }
    ui_action_confirm(confirmed);
}

int nbgl_display_address(void) {
    nbgl_useCaseAddressReview(ui_get_address(),
                              NULL,
                              &C_xeris_64px,
                              "Verify Xeris\naddress",
                              NULL,
                              address_verification_callback);
    return 0;
}

static nbgl_contentTagValue_t tx_pairs[5];
static nbgl_contentTagValueList_t tx_pair_list;

static void build_tx_review_pairs(void) {
    int idx = 0;

    tx_pairs[idx].item = "Type";
    tx_pairs[idx].value = ui_get_type();
    idx++;

    tx_pairs[idx].item = "Amount";
    tx_pairs[idx].value = ui_get_amount();
    idx++;

    tx_pairs[idx].item = "Sender";
    tx_pairs[idx].value = ui_get_sender();
    idx++;

    tx_pairs[idx].item = "Recipient";
    tx_pairs[idx].value = ui_get_recipient();
    idx++;

    tx_pairs[idx].item = "Blockhash";
    tx_pairs[idx].value = ui_get_blockhash();
    idx++;

    tx_pair_list.nbMaxLinesForValue = 0;
    tx_pair_list.nbPairs = idx;
    tx_pair_list.pairs = tx_pairs;
    tx_pair_list.smallCaseForValue = false;
    tx_pair_list.wrapping = true;
}

int nbgl_display_transaction(void) {
    build_tx_review_pairs();

    nbgl_useCaseReview(TYPE_TRANSACTION,
                       &tx_pair_list,
                       &C_xeris_64px,
                       "Review transaction\nto send Xeris",
                       NULL,
                       "Sign transaction?",
                       confirm_callback);
    return 0;
}

int nbgl_display_message(void) {
    static nbgl_contentTagValue_t msg_pair;
    static nbgl_contentTagValueList_t msg_pair_list;

    msg_pair.item = "Message";
    msg_pair.value = "Sign off-chain message";

    msg_pair_list.nbMaxLinesForValue = 0;
    msg_pair_list.nbPairs = 1;
    msg_pair_list.pairs = &msg_pair;
    msg_pair_list.smallCaseForValue = false;
    msg_pair_list.wrapping = true;

    nbgl_useCaseReview(TYPE_MESSAGE,
                       &msg_pair_list,
                       &C_xeris_64px,
                       "Review message",
                       NULL,
                       "Sign message?",
                       confirm_callback);
    return 0;
}

static const char *const info_types[] = {"Version"};
static const char *const info_contents[] = {APPVERSION};

void nbgl_menu_main(void) {
    nbgl_useCaseHomeAndSettings(APPNAME,
                                &C_xeris_64px,
                                NULL,
                                INIT_HOME_PAGE,
                                NULL,
                                info_types,
                                info_contents,
                                1,
                                NULL);
}

#endif
