#pragma once

#include <stdbool.h>

int ui_display_address(void);

int ui_display_transaction(void);

int ui_display_message(void);

void ui_action_confirm(bool approved);
void ui_action_confirm_tx(bool approved);
void ui_action_confirm_msg(bool approved);

const char *ui_get_type(void);
const char *ui_get_amount(void);
const char *ui_get_sender(void);
const char *ui_get_recipient(void);
const char *ui_get_blockhash(void);
const char *ui_get_address(void);

void ui_menu_main(void);
