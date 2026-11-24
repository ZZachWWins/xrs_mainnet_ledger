import { base58Decode, encodeCompactU16 } from "./utils";

/** A key referenced by an instruction, with its signer/writable status. */
export interface AccountMeta {
  pubkey: Buffer;
  isSigner: boolean;
  isWritable: boolean;
}

/** A single instruction within a transaction message. */
export interface Instruction {
  programId: Buffer;
  keys: AccountMeta[];
  data: Buffer;
}

/** The 32-byte zero address used as the system program ID. */
export const SYSTEM_PROGRAM_ID = Buffer.alloc(32, 0);

/**
 * Build a Xeris (Solana-compatible) transaction and serialize it
 * in on-chain wire format.
 */
export class XerisTransaction {
  private feePayer: Buffer;
  private recentBlockhash: Buffer;
  private instructions: Instruction[] = [];

  /**
   * @param feePayer 32-byte public key of the fee-paying signer.
   * @param recentBlockhash Blockhash as a hex string or 32-byte buffer.
   */
  constructor(feePayer: Buffer, recentBlockhash: string | Buffer) {
    if (feePayer.length !== 32) {
      throw new Error("feePayer must be 32 bytes");
    }
    this.feePayer = feePayer;

    if (typeof recentBlockhash === "string") {
      this.recentBlockhash = Buffer.from(recentBlockhash, "hex");
    } else {
      this.recentBlockhash = recentBlockhash;
    }
    if (this.recentBlockhash.length !== 32) {
      throw new Error("recentBlockhash must be 32 bytes");
    }
  }

  /** Append an instruction to the transaction. */
  addInstruction(
    programId: Buffer,
    keys: AccountMeta[],
    data: Buffer,
  ): void {
    this.instructions.push({ programId, keys, data });
  }

  /**
   * Serialize the transaction message (the part that gets signed).
   *
   * Wire layout (Solana v0 legacy format):
   * ```
   * [num_required_signatures : u8]
   * [num_readonly_signed     : u8]
   * [num_readonly_unsigned    : u8]
   * [num_account_keys         : compact-u16]
   * [account_keys             : 32 bytes × num_account_keys]
   * [recent_blockhash         : 32 bytes]
   * [num_instructions         : compact-u16]
   * [instructions...]
   * ```
   *
   * Each instruction:
   * ```
   * [program_id_index : u8]
   * [num_accounts     : compact-u16]
   * [account_indices  : u8 × num_accounts]
   * [data_length      : compact-u16]
   * [data             : u8 × data_length]
   * ```
   */
  serializeMessage(): Buffer {
    const { keys: accountKeys, meta } = this.buildAccountList();

    let numRequiredSignatures = 0;
    let numReadonlySigned = 0;
    let numReadonlyUnsigned = 0;

    for (const m of meta) {
      if (m.isSigner) {
        numRequiredSignatures++;
        if (!m.isWritable) numReadonlySigned++;
      } else {
        if (!m.isWritable) numReadonlyUnsigned++;
      }
    }

    const parts: Buffer[] = [];

    parts.push(Buffer.from([
      numRequiredSignatures,
      numReadonlySigned,
      numReadonlyUnsigned,
    ]));

    parts.push(encodeCompactU16(accountKeys.length));

    for (const key of accountKeys) {
      parts.push(key);
    }

    parts.push(this.recentBlockhash);

    parts.push(encodeCompactU16(this.instructions.length));

    const keyIndex = new Map<string, number>();
    for (let i = 0; i < accountKeys.length; i++) {
      keyIndex.set(accountKeys[i].toString("hex"), i);
    }

    for (const ix of this.instructions) {
      const programIdx = keyIndex.get(ix.programId.toString("hex"));
      if (programIdx === undefined) {
        throw new Error("Program ID not found in account list");
      }
      parts.push(Buffer.from([programIdx]));

      parts.push(encodeCompactU16(ix.keys.length));
      for (const key of ix.keys) {
        const idx = keyIndex.get(key.pubkey.toString("hex"));
        if (idx === undefined) {
          throw new Error("Account key not found in account list");
        }
        parts.push(Buffer.from([idx]));
      }

      parts.push(encodeCompactU16(ix.data.length));
      parts.push(ix.data);
    }

    return Buffer.concat(parts);
  }

  /**
   * Serialize the full transaction (signature placeholders + message).
   *
   * Layout:
   * ```
   * [num_signatures   : compact-u16]
   * [signatures        : 64 bytes × num_signatures (all zero placeholders)]
   * [message           : serialized message bytes]
   * ```
   */
  serialize(): Buffer {
    const message = this.serializeMessage();
    const { meta } = this.buildAccountList();
    const numSigners = meta.filter((m) => m.isSigner).length;

    const parts: Buffer[] = [];
    parts.push(encodeCompactU16(numSigners));
    for (let i = 0; i < numSigners; i++) {
      parts.push(Buffer.alloc(64, 0));
    }
    parts.push(message);

    return Buffer.concat(parts);
  }

  /**
   * Deduplicate and sort account keys following Solana ordering:
   * 1. Writable signers (fee payer first)
   * 2. Read-only signers
   * 3. Writable non-signers
   * 4. Read-only non-signers
   */
  private buildAccountList(): {
    keys: Buffer[];
    meta: { isSigner: boolean; isWritable: boolean }[];
  } {
    const map = new Map<
      string,
      { pubkey: Buffer; isSigner: boolean; isWritable: boolean }
    >();

    const feePayerHex = this.feePayer.toString("hex");
    map.set(feePayerHex, {
      pubkey: this.feePayer,
      isSigner: true,
      isWritable: true,
    });

    for (const ix of this.instructions) {
      for (const key of ix.keys) {
        const hex = key.pubkey.toString("hex");
        const existing = map.get(hex);
        if (existing) {
          existing.isSigner = existing.isSigner || key.isSigner;
          existing.isWritable = existing.isWritable || key.isWritable;
        } else {
          map.set(hex, { ...key, pubkey: Buffer.from(key.pubkey) });
        }
      }

      const pidHex = ix.programId.toString("hex");
      if (!map.has(pidHex)) {
        map.set(pidHex, {
          pubkey: Buffer.from(ix.programId),
          isSigner: false,
          isWritable: false,
        });
      }
    }

    const entries = Array.from(map.values());

    entries.sort((a, b) => {
      if (a.pubkey.equals(this.feePayer)) return -1;
      if (b.pubkey.equals(this.feePayer)) return 1;

      const aScore =
        (a.isSigner ? 0 : 2) + (a.isWritable ? 0 : 1);
      const bScore =
        (b.isSigner ? 0 : 2) + (b.isWritable ? 0 : 1);
      return aScore - bScore;
    });

    return {
      keys: entries.map((e) => e.pubkey),
      meta: entries.map((e) => ({
        isSigner: e.isSigner,
        isWritable: e.isWritable,
      })),
    };
  }
}

/**
 * Build a system-program transfer instruction (instruction type 2).
 *
 * Data layout: `[u32le type = 2] [u64le lamports]`
 */
export function createTransferInstruction(
  from: Buffer,
  to: Buffer,
  lamports: bigint,
): Instruction {
  if (from.length !== 32) throw new Error("from must be 32 bytes");
  if (to.length !== 32) throw new Error("to must be 32 bytes");
  if (lamports < 0n) throw new Error("lamports must be non-negative");

  const data = Buffer.alloc(12);
  data.writeUInt32LE(2, 0);
  data.writeBigUInt64LE(lamports, 4);

  return {
    programId: SYSTEM_PROGRAM_ID,
    keys: [
      { pubkey: from, isSigner: true, isWritable: true },
      { pubkey: to, isSigner: false, isWritable: true },
    ],
    data,
  };
}

/**
 * Build a complete transfer transaction ready for Ledger signing.
 *
 * @param from   Sender public key (32 bytes).
 * @param to     Recipient public key (32 bytes).
 * @param lamports Amount in lamports.
 * @param recentBlockhash A recent blockhash (hex string or 32-byte buffer).
 * @returns The serialized message bytes to send to the Ledger for signing.
 */
export function buildTransferMessage(
  from: Buffer,
  to: Buffer,
  lamports: bigint,
  recentBlockhash: string | Buffer,
): Buffer {
  const tx = new XerisTransaction(from, recentBlockhash);
  const ix = createTransferInstruction(from, to, lamports);
  tx.addInstruction(ix.programId, ix.keys, ix.data);
  return tx.serializeMessage();
}
