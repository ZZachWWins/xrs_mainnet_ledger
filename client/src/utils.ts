const BASE58_ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
const HARDENED_BIT = 0x80000000;
const LAMPORTS_PER_XRS = 1_000_000_000n;

/**
 * Serialize a BIP-44 path string into a binary buffer.
 *
 * Each path component is encoded as a big-endian uint32. Components
 * suffixed with `'` or `h` have the hardened bit set.
 *
 * @example serializePath("44'/9000'/0'/0'") // 4 × 4 bytes
 */
export function serializePath(path: string): Buffer {
  const stripped = path.startsWith("m/") ? path.slice(2) : path;
  const components = stripped.split("/");
  if (components.length === 0 || components.length > 10) {
    throw new Error(`Invalid derivation path: ${path}`);
  }

  const buf = Buffer.alloc(1 + components.length * 4);
  buf.writeUInt8(components.length, 0);

  for (let i = 0; i < components.length; i++) {
    let component = components[i];
    let hardened = false;

    if (component.endsWith("'") || component.endsWith("h")) {
      hardened = true;
      component = component.slice(0, -1);
    }

    const index = parseInt(component, 10);
    if (isNaN(index) || index < 0) {
      throw new Error(`Invalid path component: ${components[i]}`);
    }

    const value = hardened ? (index | HARDENED_BIT) >>> 0 : index;
    buf.writeUInt32BE(value, 1 + i * 4);
  }

  return buf;
}

/**
 * Encode bytes to a base58 string (Bitcoin alphabet).
 */
export function base58Encode(data: Buffer): string {
  if (data.length === 0) return "";

  let leadingZeros = 0;
  for (let i = 0; i < data.length && data[i] === 0; i++) {
    leadingZeros++;
  }

  let num = BigInt("0x" + data.toString("hex"));
  const chars: string[] = [];

  while (num > 0n) {
    const remainder = Number(num % 58n);
    num = num / 58n;
    chars.push(BASE58_ALPHABET[remainder]);
  }

  chars.reverse();
  return BASE58_ALPHABET[0].repeat(leadingZeros) + chars.join("");
}

/**
 * Decode a base58 string back into bytes.
 */
export function base58Decode(str: string): Buffer {
  if (str.length === 0) return Buffer.alloc(0);

  let leadingOnes = 0;
  for (let i = 0; i < str.length && str[i] === BASE58_ALPHABET[0]; i++) {
    leadingOnes++;
  }

  let num = 0n;
  for (const char of str) {
    const idx = BASE58_ALPHABET.indexOf(char);
    if (idx === -1) {
      throw new Error(`Invalid base58 character: ${char}`);
    }
    num = num * 58n + BigInt(idx);
  }

  const hex = num === 0n ? "" : num.toString(16);
  const paddedHex = hex.length % 2 === 0 ? hex : "0" + hex;
  const dataBytes = Buffer.from(paddedHex, "hex");

  return Buffer.concat([Buffer.alloc(leadingOnes), dataBytes]);
}

/**
 * Encode a u16 in Solana compact-u16 format.
 *
 * Values 0–0x7f encode as a single byte.
 * Values 0x80–0x3fff encode as two bytes.
 * Values 0x4000–0xffff encode as three bytes.
 */
export function encodeCompactU16(value: number): Buffer {
  if (value < 0 || value > 0xffff) {
    throw new Error(`compact-u16 value out of range: ${value}`);
  }

  const bytes: number[] = [];
  let remaining = value;

  while (true) {
    let elem = remaining & 0x7f;
    remaining >>= 7;
    if (remaining !== 0) {
      elem |= 0x80;
    }
    bytes.push(elem);
    if (remaining === 0) break;
  }

  return Buffer.from(bytes);
}

/**
 * Decode a Solana compact-u16 at the given offset.
 *
 * @returns A tuple of `[value, bytesConsumed]`.
 */
export function decodeCompactU16(
  buf: Buffer,
  offset: number,
): [number, number] {
  let value = 0;
  let bytesRead = 0;

  for (let shift = 0; shift < 21; shift += 7) {
    if (offset + bytesRead >= buf.length) {
      throw new Error("Unexpected end of buffer decoding compact-u16");
    }
    const byte = buf[offset + bytesRead];
    bytesRead++;
    value |= (byte & 0x7f) << shift;
    if ((byte & 0x80) === 0) break;
  }

  return [value, bytesRead];
}

/**
 * Format a lamports amount as a human-readable XRS string with 9 decimals.
 *
 * @example lamportsToXrs(1_500_000_000n) // "1.500000000"
 */
export function lamportsToXrs(lamports: bigint): string {
  const negative = lamports < 0n;
  const absVal = negative ? -lamports : lamports;

  const whole = absVal / LAMPORTS_PER_XRS;
  const frac = absVal % LAMPORTS_PER_XRS;
  const fracStr = frac.toString().padStart(9, "0");

  return (negative ? "-" : "") + whole.toString() + "." + fracStr;
}
