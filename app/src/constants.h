#pragma once

#define APPNAME     "Xeris"
#define APPVERSION  "1.0.0"
#define TICKER      "XRS"
#define DECIMALS    9
#define COIN_TYPE   9000

#define CLA_APP             0x06
#define INS_GET_VERSION     0x00
#define INS_GET_PUBKEY      0x01
#define INS_SIGN_TX         0x02
#define INS_SIGN_MESSAGE    0x03
#define INS_GET_ADDRESS     0x04

#define P1_CONFIRM    0x01
#define P1_NON_CONFIRM 0x00
#define P1_FIRST      0x00
#define P1_MORE       0x80
#define P1_LAST       0x01

#define MAX_BIP32_PATH_LEN 5
#define MAX_RAW_TX_LEN     1232
#define MAX_RAW_MSG_LEN    512

#define ADDRESS_BASE58_MAX_LEN 45
