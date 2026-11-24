# Xeris Ledger App — Security Audit Report

**App**: Xeris Ledger Signer v1.0.0  
**Chain**: Xeris L1 Mainnet (Solana fork, xeriscoin 1.0.0)  
**Date**: 2026-11-24  
**Scope**: Embedded C application, TypeScript client library, build infrastructure  

---

## Executive Summary

This audit covers the Xeris Ledger hardware wallet application designed for
signing transactions on the Xeris L1 mainnet. The app targets Ledger Stax
(primary), Nano S, Nano X, Nano S Plus, and Flex devices. The Xeris chain is
a Solana fork using Ed25519 cryptography, Solana-style binary transaction
format, and base58 addresses.

**Risk Level**: Medium (standard signing app with well-understood cryptographic
primitives on a proven hardware platform)

---

## 1. Cryptographic Review

### 1.1 Key Derivation
- **Algorithm**: Ed25519 via BIP32-Ed25519 (SLIP-0010)
- **Derivation Path**: `m/44'/9000'/account'/change'`
- **Implementation**: Uses Ledger SDK `os_derive_bip32_with_seed_no_throw()`
  with `CX_CURVE_Ed25519`
- **Finding**: PASS — Private keys are derived on the Secure Element and never
  leave it. Key material is zeroed with `explicit_bzero()` after use.

### 1.2 Transaction Signing
- **Algorithm**: Ed25519 via `cx_eddsa_sign_no_throw()`
- **Input**: Raw transaction message bytes (not a hash — Ed25519 hashes
  internally)
- **Finding**: PASS — Signing uses the Ledger SDK's audited Ed25519
  implementation. The full message is signed, not a pre-hash, which is correct
  for Ed25519.

### 1.3 Key Zeroization
- All `cx_ecfp_private_key_t` instances are zeroed in `FINALLY` blocks
- Stack-allocated key buffers use `explicit_bzero()`
- **Finding**: PASS — Sensitive material is properly cleaned up even on error
  paths.

---

## 2. Transaction Parser Security

### 2.1 Buffer Overflow Protection
- Every buffer read is bounds-checked against remaining length
- `read_compact_u16()` validates continuation bytes
- Account key count is capped at `MAX_ACCOUNT_KEYS` (32)
- Instruction count is capped at `MAX_INSTRUCTIONS` (10)
- **Finding**: PASS — All buffer accesses are validated before dereference.

### 2.2 Integer Overflow Protection
- Compact-u16 decoding validates the encoded value does not exceed 0xFFFF
- Length calculations use `size_t` to avoid 16-bit overflow
- Lamports amounts are handled as `uint64_t`
- **Finding**: PASS — No integer overflow paths identified.

### 2.3 Malformed Input Handling
- Transactions with zero signatures are rejected
- Transactions with mismatched header counts are rejected
- Instructions referencing out-of-bounds account indices are rejected
- Empty transactions return `PARSING_INVALID_LENGTH`
- **Finding**: PASS — Malformed inputs are caught and return appropriate error
  codes without crashing.

### 2.4 Blind Signing
- System program transfers display full details (amount, recipient, sender)
- Unknown program instructions are flagged as "Unknown Instruction" and
  require blind signing to be enabled in settings
- **Finding**: PASS with NOTE — Blind signing is off by default. Users must
  explicitly enable it for non-transfer instructions. This is the correct
  security posture.

---

## 3. APDU Protocol Security

### 3.1 State Machine
- App tracks state: IDLE → GET_PUBKEY or SIGN_TX
- Out-of-sequence APDUs reset state to IDLE
- Partial transaction data is cleared on state reset
- **Finding**: PASS — State machine prevents mixing operations.

### 3.2 Chunked Transfer
- First chunk (P1=0x00) resets accumulation buffer and parses BIP32 path
- Continuation chunks (P1=0x80) append to buffer with overflow check
- Total transaction size capped at `MAX_TX_SIZE` (1232 bytes)
- **Finding**: PASS — Chunked protocol handles boundary conditions correctly.

### 3.3 Input Validation
- CLA byte must be 0x06 (Xeris app class)
- INS byte is validated against known commands
- P1/P2 values are checked for each instruction
- Data length is validated against minimum requirements
- **Finding**: PASS — All APDU fields are validated before processing.

---

## 4. Display Security

### 4.1 Address Verification
- Full base58 address is displayed on-device for user verification
- Address is computed from the derived public key on-device, not received from
  the host
- **Finding**: PASS — Address display prevents address substitution attacks.

### 4.2 Transaction Review
- Amount is displayed in human-readable format with proper decimal handling
- Recipient address is shown in full base58
- Recent blockhash is displayed (truncated for readability)
- Transaction type is identified and shown
- **Finding**: PASS — All critical transaction fields are displayed for user
  verification.

### 4.3 Display Overflow
- All `snprintf` calls use bounded output buffers
- Base58 encoding output is bounded
- Amount formatting handles maximum uint64 values
- **Finding**: PASS — No display buffer overflows identified.

---

## 5. TypeScript Client Library

### 5.1 Transport Security
- Communicates via standard `@ledgerhq/hw-transport` (USB HID or WebUSB)
- No raw USB access — relies on audited Ledger transport layer
- **Finding**: PASS

### 5.2 Serialization
- Transaction serialization follows Solana wire format exactly
- Compact-u16 encoding matches the embedded parser's expectations
- **Finding**: PASS — Client and device agree on wire format.

### 5.3 Input Validation
- BIP32 path strings are parsed and validated before serialization
- Transaction parameters are type-checked
- Buffer sizes are validated before APDU construction
- **Finding**: PASS

---

## 6. Build and Supply Chain

### 6.1 Reproducible Builds
- Docker-based build using `ghcr.io/ledgerhq/ledger-app-builder`
- Pinned SDK version in Makefile
- **Finding**: PASS — Builds are reproducible via Docker.

### 6.2 Dependencies
- Embedded app: zero external dependencies (SDK only)
- Client library: minimal dependencies (@ledgerhq/hw-transport, bs58)
- **Finding**: PASS — Minimal attack surface.

### 6.3 CI/CD
- GitHub Actions runs build, lint, and tests on every PR
- Static analysis via cppcheck
- npm audit for client dependencies
- **Finding**: PASS

---

## 7. Recommendations

### 7.1 Before Mainnet Deployment
1. **Register SLIP-0044 coin type**: File a PR to
   github.com/satoshilabs/slips to register coin type 9000 for Xeris
2. **Fuzz testing**: Run AFL/libFuzzer on the transaction parser for at least
   24 hours before release
3. **Speculos testing**: Run the full functional test suite against the
   Speculos emulator for all target devices
4. **Third-party audit**: Engage a professional security auditor (e.g.,
   Kudelski, Trail of Bits) for a formal review before Ledger Live submission

### 7.2 Ongoing
1. Monitor Xeris protocol changes that may affect transaction format
2. Keep Ledger SDK updated for security patches
3. Run CI on every commit
4. Consider adding token (SPL-equivalent) transfer display support

---

## 8. Checklist Summary

| Category | Status | Notes |
|---|---|---|
| Key derivation | PASS | Ed25519 via SLIP-0010 |
| Key zeroization | PASS | explicit_bzero in FINALLY |
| Buffer overflow | PASS | All reads bounds-checked |
| Integer overflow | PASS | Proper types used |
| State machine | PASS | Reset on sequence error |
| APDU validation | PASS | CLA/INS/P1/P2/LC checked |
| Display security | PASS | Full address + amount shown |
| Blind signing | PASS | Off by default |
| Transport | PASS | Ledger SDK transport |
| Build chain | PASS | Reproducible Docker build |
| Dependencies | PASS | Minimal surface |

**Overall Assessment**: The Xeris Ledger app follows security best practices
for hardware wallet applications. It is suitable for mainnet deployment after
completing the recommendations in section 7.1.
