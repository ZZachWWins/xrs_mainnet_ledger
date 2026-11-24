#include <string.h>
#include "key_derivation.h"
#include "os.h"
#include "cx.h"

int crypto_derive_public_key(const bip32_path_t *path, uint8_t raw_pubkey[32]) {
    cx_ecfp_private_key_t private_key = {0};
    cx_ecfp_public_key_t public_key = {0};
    uint8_t raw_private[64] = {0};

    BEGIN_TRY {
        TRY {
            os_perso_derive_node_bip32_seed_key(HDW_ED25519_SLIP10,
                                                CX_CURVE_Ed25519,
                                                path->path,
                                                path->path_len,
                                                raw_private,
                                                NULL,
                                                NULL,
                                                0);

            cx_ecfp_init_private_key_no_throw(CX_CURVE_Ed25519,
                                              raw_private,
                                              32,
                                              &private_key);

            cx_ecfp_init_public_key_no_throw(CX_CURVE_Ed25519,
                                             NULL,
                                             0,
                                             &public_key);

            cx_ecfp_generate_pair_no_throw(CX_CURVE_Ed25519,
                                           &public_key,
                                           &private_key,
                                           1);

            // Ed25519 public key from BOLOS is 65 bytes (04 || X || Y).
            // Xeris (like Solana) uses the 32-byte compressed form (Y coordinate).
            // BOLOS stores Y in big-endian in bytes 33..64.
            for (int i = 0; i < 32; i++) {
                raw_pubkey[i] = public_key.W[64 - i];
            }
            if (public_key.W[32] & 1) {
                raw_pubkey[31] |= 0x80;
            }
        }
        CATCH_OTHER(e) {
            explicit_bzero(raw_private, sizeof(raw_private));
            explicit_bzero(&private_key, sizeof(private_key));
            return -1;
        }
        FINALLY {
            explicit_bzero(raw_private, sizeof(raw_private));
            explicit_bzero(&private_key, sizeof(private_key));
        }
    }
    END_TRY;

    return 0;
}

int crypto_sign_message(const bip32_path_t *path,
                        const uint8_t *msg,
                        size_t msg_len,
                        uint8_t sig[64]) {
    cx_ecfp_private_key_t private_key = {0};
    uint8_t raw_private[64] = {0};
    size_t sig_len = 64;

    BEGIN_TRY {
        TRY {
            os_perso_derive_node_bip32_seed_key(HDW_ED25519_SLIP10,
                                                CX_CURVE_Ed25519,
                                                path->path,
                                                path->path_len,
                                                raw_private,
                                                NULL,
                                                NULL,
                                                0);

            cx_ecfp_init_private_key_no_throw(CX_CURVE_Ed25519,
                                              raw_private,
                                              32,
                                              &private_key);

            cx_eddsa_sign_no_throw(&private_key,
                                   CX_SHA512,
                                   msg,
                                   msg_len,
                                   sig,
                                   sig_len);
        }
        CATCH_OTHER(e) {
            explicit_bzero(raw_private, sizeof(raw_private));
            explicit_bzero(&private_key, sizeof(private_key));
            return -1;
        }
        FINALLY {
            explicit_bzero(raw_private, sizeof(raw_private));
            explicit_bzero(&private_key, sizeof(private_key));
        }
    }
    END_TRY;

    return 0;
}
