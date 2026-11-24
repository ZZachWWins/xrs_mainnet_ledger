# Xeris Ledger App Architecture

## Overview

The Xeris Ledger app provides hardware wallet signing for the Xeris L1
blockchain. Xeris is a Solana fork (solana-core 1.0.0, xeriscoin 1.0.0) using
Ed25519 cryptography and Solana-compatible transaction formats.

## Chain Specifications

| Property | Value |
|---|---|
| Chain | Xeris L1 Mainnet |
| Base Protocol | Solana fork |
| Signing Algorithm | Ed25519 |
| Address Format | Base58 (32-byte public key) |
| Native Token | XRS |
| Decimals | 9 (lamports) |
| BIP44 Coin Type | 9000 |
| Mainnet RPC | mainnetrpc.xerisweb.com |
| Max Transaction Size | 1232 bytes |

## Component Architecture

```
┌──────────────────────────────────────────────────┐
│                  Host Computer                    │
│                                                   │
│  ┌─────────────────────────────────────────────┐  │
│  │         TypeScript Client Library            │  │
│  │  (@xeris/ledger-app)                        │  │
│  │                                              │  │
│  │  ┌──────────┐ ┌────────────┐ ┌───────────┐  │  │
│  │  │ XerisLedger│ │Serialization│ │  Utils   │  │  │
│  │  │  Class    │ │  Module    │ │ (base58) │  │  │
│  │  └────┬─────┘ └────────────┘ └───────────┘  │  │
│  │       │                                      │  │
│  │  ┌────▼─────────────────────────────────┐    │  │
│  │  │  @ledgerhq/hw-transport              │    │  │
│  │  │  (USB HID / WebUSB / Bluetooth)      │    │  │
│  │  └────┬─────────────────────────────────┘    │  │
│  └───────┼──────────────────────────────────────┘  │
│          │  APDU Protocol                          │
└──────────┼─────────────────────────────────────────┘
           │
┌──────────▼─────────────────────────────────────────┐
│              Ledger Hardware Device                  │
│                                                     │
│  ┌───────────────────────────────────────────────┐  │
│  │          BOLOS Application (C)                 │  │
│  │                                                │  │
│  │  ┌──────────┐  ┌───────────┐  ┌───────────┐   │  │
│  │  │  APDU    │  │Transaction│  │  Crypto   │   │  │
│  │  │Dispatcher│──│  Parser   │  │  Module   │   │  │
│  │  └──┬───┬──┘  └───────────┘  └───────────┘   │  │
│  │     │   │                                      │  │
│  │  ┌──▼───▼──────────────────────────────────┐   │  │
│  │  │  UI Display (NBGL for Stax / BAGL)      │   │  │
│  │  └─────────────────────────────────────────┘   │  │
│  │                                                │  │
│  │  ┌─────────────────────────────────────────┐   │  │
│  │  │  Secure Element (Ed25519 keys)          │   │  │
│  │  └─────────────────────────────────────────┘   │  │
│  └───────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
```

## APDU Protocol

Communication uses ISO 7816-4 APDU format:

### Commands

| INS | Name | Description |
|-----|------|-------------|
| 0x00 | GET_VERSION | Returns app version (3 bytes) |
| 0x01 | GET_PUBKEY | Derive and return Ed25519 public key |
| 0x02 | SIGN_TX | Sign a Xeris transaction |
| 0x03 | SIGN_MESSAGE | Sign an arbitrary message |

### APDU Structure

**Request**: `[CLA=0x06] [INS] [P1] [P2] [LC] [DATA...]`

**Response**: `[DATA...] [SW1] [SW2]`

### GET_PUBKEY (0x01)

Request:
- P1: 0x00 = return only, 0x01 = display on device
- P2: 0x00
- Data: BIP32 path (1 byte length + 4 bytes per component)

Response:
- 32 bytes: raw Ed25519 public key

### SIGN_TX (0x02)

Chunked protocol for transactions up to 1232 bytes:

First chunk (P1=0x00):
- Data: BIP32 path + first N bytes of transaction

Continuation chunks (P1=0x80):
- Data: next N bytes of transaction

Final response (after user approval):
- 64 bytes: Ed25519 signature

### SIGN_MESSAGE (0x03)

Same chunked protocol as SIGN_TX but for arbitrary messages.

## BIP44 Derivation

Standard path: `m/44'/9000'/account'/change'`

- Purpose: 44' (standard)
- Coin Type: 9000' (Xeris — pending SLIP-0044 registration)
- Account: 0' (default)
- Change: 0' (external)

## Transaction Format

Xeris uses the Solana binary transaction format:

```
Transaction:
  ├── num_signatures (compact-u16)
  ├── signatures[] (64 bytes each)
  └── Message:
      ├── Header:
      │   ├── num_required_signatures (u8)
      │   ├── num_readonly_signed (u8)
      │   └── num_readonly_unsigned (u8)
      ├── account_keys[] (compact-u16 count, 32 bytes each)
      ├── recent_blockhash (32 bytes)
      └── instructions[] (compact-u16 count):
          ├── program_id_index (u8)
          ├── account_indices[] (compact-u16 count, u8 each)
          └── data[] (compact-u16 count, raw bytes)
```

### System Program Transfer

Identified when program_id is all zeros and instruction data starts with
`[2, 0, 0, 0]` (little-endian u32 = 2 for transfer), followed by 8 bytes
of little-endian u64 lamports amount.

## Security Model

1. **Private keys never leave the Secure Element**
2. **All transactions require user confirmation on-device display**
3. **Blind signing is off by default** — unknown instructions must be
   explicitly enabled
4. **Key material is zeroed after every operation**
5. **All buffer accesses are bounds-checked**
6. **State machine prevents operation mixing**

## Device Support

| Device | UI Framework | Icon Size | Notes |
|--------|-------------|-----------|-------|
| Nano S | BAGL | 16x16 | Limited memory |
| Nano S Plus | BAGL | 14x14 | |
| Nano X | BAGL | 14x14 | Bluetooth |
| Stax | NBGL | 32x32 | Touchscreen primary |
| Flex | NBGL | 40x40 | Touchscreen |

## Build

```bash
# Docker build (recommended)
docker run --rm -v $(pwd):/app ghcr.io/ledgerhq/ledger-app-builder/ledger-app-builder:latest \
  make -C app

# Client library
cd client && npm install && npm run build
```
