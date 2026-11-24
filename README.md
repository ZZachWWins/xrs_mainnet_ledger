# Xeris Ledger App

Hardware wallet application for the **Xeris L1 mainnet** blockchain, targeting Ledger Stax, Flex, Nano S, Nano S Plus, and Nano X.

Xeris is a Solana-fork blockchain (solana-core 1.0.0, xeriscoin 1.0.0) using Ed25519 cryptography, Solana-compatible binary transaction format, and base58 addresses.

## Features

- **Ed25519 key derivation** via BIP32-Ed25519 (SLIP-0010) at path `m/44'/9000'/account'/change'`
- **Transaction signing** with on-device review of amount, recipient, and type
- **Message signing** for off-chain message authentication
- **Address verification** — display and confirm address on device
- **Blind signing protection** — unknown instruction types require explicit opt-in
- **Multi-device support** — Stax/Flex (NBGL touchscreen), Nano S/S+/X (BAGL buttons)

## Chain Details

| Property | Value |
|---|---|
| Network | Xeris L1 Mainnet |
| RPC | `mainnetrpc.xerisweb.com` |
| Ticker | XRS |
| Decimals | 9 (lamports) |
| Signing | Ed25519 |
| Address Format | Base58 |
| BIP44 Coin Type | 9000 |

## Repository Structure

```
xrs_mainnet_ledger/
├── app/                    # Embedded C application (BOLOS)
│   ├── src/
│   │   ├── apdu/           # APDU command handlers
│   │   ├── crypto/         # Ed25519 key derivation and signing
│   │   ├── transaction/    # Solana-style transaction parser
│   │   └── ui/             # Device display (NBGL + BAGL)
│   ├── glyphs/             # App icons for all devices
│   └── Makefile
├── client/                 # TypeScript client library
│   ├── src/
│   │   ├── Xeris.ts        # Main XerisLedger class
│   │   ├── serialization.ts # Transaction wire format
│   │   └── utils.ts        # Base58, path serialization
│   └── tests/
├── tests/
│   ├── functional/         # Speculos emulator tests (Python)
│   ├── unit/               # C unit tests for parser
│   └── fuzzing/            # AFL/libFuzzer harness
├── docs/
│   ├── ARCHITECTURE.md     # Technical architecture
│   ├── SECURITY_AUDIT.md   # Security audit report
│   └── LEDGER_LIVE_SUBMISSION.md  # Publishing guide
├── .github/workflows/      # CI/CD
├── ledger_app.toml         # Ledger build manifest
└── scripts/install.sh      # Device installation helper
```

## Building

### Embedded App (Docker — recommended)

```bash
# Build for Ledger Stax
docker run --rm -v $(pwd):/app \
  ghcr.io/ledgerhq/ledger-app-builder/ledger-app-builder:latest \
  make -C app BOLOS_SDK=$STAX_SDK

# Build for Nano S
docker run --rm -v $(pwd):/app \
  ghcr.io/ledgerhq/ledger-app-builder/ledger-app-builder:latest \
  make -C app BOLOS_SDK=$NANOS_SDK
```

### Client Library

```bash
cd client
npm install
npm run build
```

## Testing

### Client Library Tests

```bash
cd client
npm test
```

### Functional Tests (Speculos)

```bash
pip install -r tests/functional/requirements.txt
pytest tests/functional/ --device stax
```

### Fuzzing

```bash
cd tests/fuzzing
# With libFuzzer
clang -fsanitize=fuzzer,address -I../../app/src \
  -o fuzz_parser fuzz_parser.c ../../app/src/transaction/parser.c
./fuzz_parser corpus/ -max_total_time=3600
```

## Usage

### TypeScript/JavaScript

```typescript
import { XerisLedger } from "@xeris/ledger-app";
import TransportNodeHid from "@ledgerhq/hw-transport-node-hid";

const transport = await TransportNodeHid.create();
const xeris = new XerisLedger(transport);

// Get app version
const version = await xeris.getVersion();
console.log(`Xeris app v${version.major}.${version.minor}.${version.patch}`);

// Get address (display on device for verification)
const { publicKey, address } = await xeris.getAddress("44'/9000'/0'/0'", true);
console.log(`Address: ${address}`);

// Sign a transaction
import { XerisTransaction, createTransferInstruction } from "@xeris/ledger-app";

const tx = new XerisTransaction(
  senderPubkey,    // 32-byte Buffer
  recentBlockhash  // 32-byte Buffer
);
tx.addInstruction(createTransferInstruction(senderPubkey, recipientPubkey, 1_000_000_000n));

const message = tx.serializeMessage();
const { signature } = await xeris.signTransaction("44'/9000'/0'/0'", message);
```

### Web (WebUSB)

```typescript
import { XerisLedger } from "@xeris/ledger-app";
import TransportWebUSB from "@ledgerhq/hw-transport-webusb";

const transport = await TransportWebUSB.create();
const xeris = new XerisLedger(transport);
```

## APDU Protocol

| INS | Command | Description |
|-----|---------|-------------|
| 0x00 | GET_VERSION | Returns 3-byte version |
| 0x01 | GET_PUBKEY | Derive Ed25519 public key |
| 0x02 | SIGN_TX | Sign transaction (chunked) |
| 0x03 | SIGN_MESSAGE | Sign arbitrary message (chunked) |

CLA: `0x06`

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for full protocol specification.

## Security

- Private keys never leave the Secure Element
- All transactions require on-device user confirmation
- Blind signing is disabled by default
- All buffer accesses are bounds-checked
- Key material is zeroed after every operation

See [docs/SECURITY_AUDIT.md](docs/SECURITY_AUDIT.md) for the full audit report.

## Publishing to Ledger Live

See [docs/LEDGER_LIVE_SUBMISSION.md](docs/LEDGER_LIVE_SUBMISSION.md) for the
complete submission guide, checklist, and timeline.

## License

Apache License 2.0 — see [LICENSE](LICENSE).
