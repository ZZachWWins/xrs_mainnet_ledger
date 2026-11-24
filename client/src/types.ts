/** APDU class byte for the Xeris Ledger app. */
export const CLA = 0x06;

/** APDU instruction codes. */
export enum INS {
  GET_VERSION = 0x00,
  GET_PUBKEY = 0x01,
  SIGN_TX = 0x02,
  SIGN_MESSAGE = 0x03,
}

/** P1 flags for GET_PUBKEY / signing instructions. */
export enum P1 {
  NON_CONFIRM = 0x00,
  CONFIRM = 0x01,
  MORE = 0x80,
}

/** P2 is currently unused; always zero. */
export const P2_DEFAULT = 0x00;

/** Maximum APDU data payload per chunk. */
export const CHUNK_SIZE = 255;

/** Length of an Ed25519 public key in bytes. */
export const PUBKEY_LENGTH = 32;

/** Length of an Ed25519 signature in bytes. */
export const SIGNATURE_LENGTH = 64;

/** Default BIP-44 derivation path for Xeris (coin type 9000). */
export const DEFAULT_PATH = "44'/9000'/0'/0'";

/** Xeris app version as reported by the device. */
export interface XerisVersion {
  major: number;
  minor: number;
  patch: number;
}

/** Public key and its base58-encoded address. */
export interface XerisAddress {
  publicKey: Buffer;
  address: string;
}

/** Ed25519 signature returned after signing. */
export interface XerisSignature {
  signature: Buffer;
}

/** Parameters for a native XRS transfer instruction. */
export interface TransferParams {
  from: string;
  to: string;
  lamports: bigint;
  recentBlockhash: string;
}

/** BIP-44 derivation path in string form (e.g. "44'/9000'/0'/0'"). */
export type DerivationPath = string;

/** APDU status words. */
export enum StatusWord {
  OK = 0x9000,
  DENY = 0x6985,
  WRONG_P1P2 = 0x6a86,
  WRONG_DATA_LENGTH = 0x6a87,
  INS_NOT_SUPPORTED = 0x6d00,
  CLA_NOT_SUPPORTED = 0x6e00,
  TECHNICAL_PROBLEM = 0x6f00,
  WRONG_RESPONSE_LENGTH = 0xb000,
  DISPLAY_ADDRESS_FAIL = 0xb002,
  TX_PARSING_FAIL = 0xb005,
  TX_HASH_FAIL = 0xb006,
  BAD_STATE = 0xb007,
  SIGNATURE_FAIL = 0xb008,
}
