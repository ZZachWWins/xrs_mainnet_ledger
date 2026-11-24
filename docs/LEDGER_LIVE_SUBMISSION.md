# Ledger Live Submission Guide — Xeris App

This guide walks through the process of getting the Xeris Ledger app published
on Ledger Live.

---

## Prerequisites

Before submitting to Ledger:

1. **SLIP-0044 Coin Type Registration**
   - File a PR to https://github.com/satoshilabs/slips/blob/master/slip-0044.md
   - Register coin type `9000` for "Xeris" / "XRS"
   - This is required for Ledger to accept the app

2. **Working App**
   - App builds for all target devices (Nano S, S+, X, Stax, Flex)
   - All functional tests pass on Speculos emulator
   - Security audit completed (see docs/SECURITY_AUDIT.md)

3. **Professional Icons**
   - Replace placeholder BMP icons in `app/glyphs/` with professional Xeris
     branding
   - Nano S: 16x16 1-bit BMP
   - Nano S+/X: 14x14 1-bit BMP
   - Stax: 32x32 NBGL icon
   - Flex: 40x40 NBGL icon

4. **Third-Party Security Audit** (recommended)
   - Ledger strongly recommends a professional audit for mainnet apps
   - Suggested firms: Kudelski Security, Trail of Bits, Halborn

---

## Submission Process

### Step 1: Review Ledger Developer Documentation

Read the official guidelines:
- https://developers.ledger.com/docs/device-app/deliver/deliverables/documentation
- https://developers.ledger.com/docs/device-app/deliver/process

### Step 2: Prepare the Ledger App Manifest

The `ledger_app.toml` in the repository root defines the app metadata for
Ledger's build infrastructure. Verify it contains:

```toml
[app]
build_directory = "app"
sdk = "C"
devices = ["nanos", "nanox", "nanosp", "stax", "flex"]
```

### Step 3: Submit to Ledger

1. **Fork** the `ledger-app-database` repository:
   https://github.com/LedgerHQ/ledger-app-database

2. **Add an entry** for the Xeris app in the database JSON:
   ```json
   {
     "name": "Xeris",
     "ticker": "XRS",
     "category": "coin",
     "chain": "xeris",
     "repository": "https://github.com/zzachwwins/xrs_mainnet_ledger",
     "devices": ["nanos", "nanox", "nanosp", "stax", "flex"]
   }
   ```

3. **Open a Pull Request** to the ledger-app-database repository

4. **Ledger Review Process**:
   - Ledger's team will review the PR
   - They will clone and build the app
   - They will run security checks
   - They may request changes

### Step 4: Ledger Security Review

Ledger performs their own security assessment:

1. **Automated Checks**: Build reproducibility, static analysis
2. **Manual Review**: Code review by Ledger security team
3. **Functional Testing**: Against Speculos and real hardware
4. **Penetration Testing**: For high-value apps

Expected timeline: 2-6 weeks for initial review.

### Step 5: Ledger Live Integration

After app approval:

1. **Add Ledger Live support** by contributing to the
   `ledger-live` monorepo:
   https://github.com/LedgerHQ/ledger-live

2. Create a **coin module** that implements:
   - Account synchronization (via Xeris RPC: mainnetrpc.xerisweb.com)
   - Transaction building and broadcasting
   - Balance display
   - Transaction history

3. Register the coin in Ledger Live's **crypto-assets**:
   https://github.com/LedgerHQ/crypto-assets

---

## Deliverables Checklist

| Item | Status | Notes |
|------|--------|-------|
| App source code | Done | This repository |
| ledger_app.toml | Done | Root of repository |
| App icons (all devices) | Placeholder | Replace with branded icons |
| Functional tests | Done | tests/functional/ |
| Unit tests | Done | tests/unit/ |
| Security audit | Done | docs/SECURITY_AUDIT.md |
| SLIP-0044 registration | TODO | File PR for coin type 9000 |
| Third-party audit | Recommended | Not yet started |
| Ledger Live coin module | TODO | After app approval |
| ledger-app-database PR | TODO | After app approval |
| crypto-assets registration | TODO | After Ledger Live module |

---

## Estimated Timeline

| Phase | Duration | Notes |
|-------|----------|-------|
| SLIP-0044 registration | 1-2 weeks | PR review by maintainers |
| Icon design | 1 week | Professional branding |
| Third-party audit | 2-4 weeks | Optional but recommended |
| Ledger submission | 1 day | PR to ledger-app-database |
| Ledger security review | 2-6 weeks | Depends on queue |
| Ledger Live module | 2-3 weeks | Coin module development |
| Ledger Live integration | 1-2 weeks | PR review |
| **Total (optimistic)** | **6-8 weeks** | |
| **Total (conservative)** | **12-16 weeks** | |

---

## Contacts

- **Ledger Developer Portal**: https://developers.ledger.com
- **Ledger Developer Slack**: https://ledger-dev.slack.com
- **Xeris Mainnet RPC**: https://mainnetrpc.xerisweb.com
- **Repository**: https://github.com/zzachwwins/xrs_mainnet_ledger

---

## Common Rejection Reasons

To avoid delays, ensure:

1. **No blind signing by default** — the app must display transaction details
2. **All paths hardened** — BIP32 path must use hardened derivation
3. **Proper error handling** — all error codes from SDK calls are checked
4. **Memory safety** — no buffer overflows, use-after-free, or leaks
5. **Clean build** — no warnings with -Wall -Wextra -Werror
6. **Reproducible** — Docker build must produce identical binary
7. **Icons correct format** — exact pixel dimensions and bit depth
8. **Version numbering** — semantic versioning in Makefile
