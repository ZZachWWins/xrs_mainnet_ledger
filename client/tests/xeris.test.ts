import {
  base58Decode,
  base58Encode,
  decodeCompactU16,
  encodeCompactU16,
  lamportsToXrs,
  serializePath,
} from "../src/utils";
import {
  XerisTransaction,
  createTransferInstruction,
  SYSTEM_PROGRAM_ID,
  buildTransferMessage,
} from "../src/serialization";
import { XerisLedger } from "../src/Xeris";
import { CLA, INS, P1, P2_DEFAULT, StatusWord } from "../src/types";

// --- Mock transport ---

class MockTransport {
  responses: Buffer[] = [];
  sentAPDUs: { cla: number; ins: number; p1: number; p2: number; data?: Buffer }[] = [];

  pushResponse(data: Buffer, sw: number = StatusWord.OK): void {
    const swBuf = Buffer.alloc(2);
    swBuf.writeUInt16BE(sw, 0);
    this.responses.push(Buffer.concat([data, swBuf]));
  }

  async send(
    cla: number,
    ins: number,
    p1: number,
    p2: number,
    data?: Buffer,
    _statusList?: number[],
  ): Promise<Buffer> {
    this.sentAPDUs.push({ cla, ins, p1, p2, data });
    const response = this.responses.shift();
    if (!response) throw new Error("No mock response queued");
    return response;
  }
}

// --- serializePath ---

describe("serializePath", () => {
  it("serializes 44'/9000'/0'/0' correctly", () => {
    const buf = serializePath("44'/9000'/0'/0'");
    expect(buf.length).toBe(1 + 4 * 4);
    expect(buf[0]).toBe(4); // 4 components
    expect(buf.readUInt32BE(1)).toBe((44 | 0x80000000) >>> 0);
    expect(buf.readUInt32BE(5)).toBe((9000 | 0x80000000) >>> 0);
    expect(buf.readUInt32BE(9)).toBe((0 | 0x80000000) >>> 0);
    expect(buf.readUInt32BE(13)).toBe((0 | 0x80000000) >>> 0);
  });

  it("handles m/ prefix", () => {
    const a = serializePath("m/44'/9000'/0'/0'");
    const b = serializePath("44'/9000'/0'/0'");
    expect(a.equals(b)).toBe(true);
  });

  it("handles non-hardened components", () => {
    const buf = serializePath("44'/9000'/0'/0");
    expect(buf.readUInt32BE(13)).toBe(0); // no hardened bit
  });

  it("throws on empty path", () => {
    expect(() => serializePath("")).toThrow();
  });
});

// --- base58 ---

describe("base58", () => {
  it("roundtrips a known value", () => {
    const original = Buffer.from(
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef",
      "hex",
    );
    const encoded = base58Encode(original);
    const decoded = base58Decode(encoded);
    expect(decoded.equals(original)).toBe(true);
  });

  it("handles leading zeros", () => {
    const data = Buffer.from("0000ff", "hex");
    const encoded = base58Encode(data);
    expect(encoded.startsWith("11")).toBe(true); // two leading '1's
    const decoded = base58Decode(encoded);
    expect(decoded.equals(data)).toBe(true);
  });

  it("handles empty buffer", () => {
    expect(base58Encode(Buffer.alloc(0))).toBe("");
    expect(base58Decode("").length).toBe(0);
  });

  it("throws on invalid character", () => {
    expect(() => base58Decode("0OIl")).toThrow("Invalid base58 character");
  });
});

// --- compact-u16 ---

describe("compact-u16", () => {
  const cases: [number, number[]][] = [
    [0, [0x00]],
    [1, [0x01]],
    [127, [0x7f]],
    [128, [0x80, 0x01]],
    [255, [0xff, 0x01]],
    [16383, [0xff, 0x7f]],
    [16384, [0x80, 0x80, 0x01]],
  ];

  it.each(cases)("encodes %i correctly", (value, expected) => {
    const buf = encodeCompactU16(value);
    expect([...buf]).toEqual(expected);
  });

  it.each(cases)("decodes %i correctly", (value, bytes) => {
    const buf = Buffer.from(bytes);
    const [decoded, bytesRead] = decodeCompactU16(buf, 0);
    expect(decoded).toBe(value);
    expect(bytesRead).toBe(bytes.length);
  });

  it("roundtrips all values up to 300", () => {
    for (let i = 0; i <= 300; i++) {
      const encoded = encodeCompactU16(i);
      const [decoded] = decodeCompactU16(encoded, 0);
      expect(decoded).toBe(i);
    }
  });

  it("throws on out-of-range", () => {
    expect(() => encodeCompactU16(-1)).toThrow();
    expect(() => encodeCompactU16(0x10000)).toThrow();
  });

  it("decodes at non-zero offset", () => {
    const buf = Buffer.from([0xaa, 0x80, 0x01, 0xbb]);
    const [val, read] = decodeCompactU16(buf, 1);
    expect(val).toBe(128);
    expect(read).toBe(2);
  });
});

// --- lamportsToXrs ---

describe("lamportsToXrs", () => {
  it("formats whole XRS", () => {
    expect(lamportsToXrs(1_000_000_000n)).toBe("1.000000000");
  });

  it("formats fractional", () => {
    expect(lamportsToXrs(1_500_000_000n)).toBe("1.500000000");
  });

  it("formats zero", () => {
    expect(lamportsToXrs(0n)).toBe("0.000000000");
  });

  it("formats sub-lamport amounts", () => {
    expect(lamportsToXrs(1n)).toBe("0.000000001");
  });

  it("formats large amounts", () => {
    expect(lamportsToXrs(123_456_789_012_345_678n)).toBe(
      "123456789.012345678",
    );
  });
});

// --- XerisTransaction serialization ---

describe("XerisTransaction", () => {
  const from = Buffer.alloc(32, 0x01);
  const to = Buffer.alloc(32, 0x02);
  const blockhash = Buffer.alloc(32, 0xaa);

  it("serializes a transfer message", () => {
    const msg = buildTransferMessage(from, to, 1_000_000_000n, blockhash);
    expect(msg.length).toBeGreaterThan(0);

    // Header: 3 bytes
    expect(msg[0]).toBe(1); // 1 required signer (from)
    expect(msg[1]).toBe(0); // 0 readonly signed
    // msg[2] = readonly unsigned count (system program is readonly unsigned)
    expect(msg[2]).toBe(1);
  });

  it("includes all accounts in correct order", () => {
    const tx = new XerisTransaction(from, blockhash);
    const ix = createTransferInstruction(from, to, 500n);
    tx.addInstruction(ix.programId, ix.keys, ix.data);
    const msg = tx.serializeMessage();

    // After 3-byte header + compact-u16 for num accounts:
    // accounts = [from (signer+writable), to (writable), system (readonly)]
    const [numAccounts, compactLen] = readCompactU16(msg, 3);
    expect(numAccounts).toBe(3);

    const firstKey = msg.subarray(3 + compactLen, 3 + compactLen + 32);
    expect(firstKey.equals(from)).toBe(true); // fee payer first
  });

  it("serialize() includes signature placeholders", () => {
    const tx = new XerisTransaction(from, blockhash);
    const ix = createTransferInstruction(from, to, 100n);
    tx.addInstruction(ix.programId, ix.keys, ix.data);
    const full = tx.serialize();

    // First byte: compact-u16 for 1 signature
    const [numSigs, compactLen] = readCompactU16(full, 0);
    expect(numSigs).toBe(1);

    // 64 zero bytes for the placeholder
    const sigSlot = full.subarray(compactLen, compactLen + 64);
    expect(sigSlot.every((b) => b === 0)).toBe(true);
  });

  it("throws on invalid feePayer length", () => {
    expect(() => new XerisTransaction(Buffer.alloc(16), blockhash)).toThrow();
  });

  it("throws on invalid blockhash length", () => {
    expect(() => new XerisTransaction(from, Buffer.alloc(16))).toThrow();
  });
});

// --- createTransferInstruction ---

describe("createTransferInstruction", () => {
  it("creates correct instruction data", () => {
    const from = Buffer.alloc(32, 0x01);
    const to = Buffer.alloc(32, 0x02);
    const ix = createTransferInstruction(from, to, 42n);

    expect(ix.programId.equals(SYSTEM_PROGRAM_ID)).toBe(true);
    expect(ix.keys.length).toBe(2);
    expect(ix.keys[0].isSigner).toBe(true);
    expect(ix.keys[0].isWritable).toBe(true);
    expect(ix.keys[1].isSigner).toBe(false);
    expect(ix.keys[1].isWritable).toBe(true);

    // Data: u32le type=2, u64le lamports=42
    expect(ix.data.readUInt32LE(0)).toBe(2);
    expect(ix.data.readBigUInt64LE(4)).toBe(42n);
  });

  it("throws on invalid key lengths", () => {
    expect(() =>
      createTransferInstruction(Buffer.alloc(16), Buffer.alloc(32), 1n),
    ).toThrow();
  });

  it("throws on negative lamports", () => {
    expect(() =>
      createTransferInstruction(Buffer.alloc(32), Buffer.alloc(32), -1n),
    ).toThrow();
  });
});

// --- XerisLedger (with mock transport) ---

describe("XerisLedger", () => {
  it("getVersion parses response", async () => {
    const mock = new MockTransport();
    mock.pushResponse(Buffer.from([1, 2, 3]));

    const xeris = new XerisLedger(mock as any);
    const version = await xeris.getVersion();

    expect(version).toEqual({ major: 1, minor: 2, patch: 3 });
    expect(mock.sentAPDUs[0].ins).toBe(INS.GET_VERSION);
  });

  it("getAddress returns pubkey and base58 address", async () => {
    const mock = new MockTransport();
    const pubkey = Buffer.alloc(32, 0xab);
    mock.pushResponse(pubkey);

    const xeris = new XerisLedger(mock as any);
    const result = await xeris.getAddress("44'/9000'/0'/0'");

    expect(result.publicKey.equals(pubkey)).toBe(true);
    expect(typeof result.address).toBe("string");
    expect(result.address.length).toBeGreaterThan(0);
    expect(mock.sentAPDUs[0].ins).toBe(INS.GET_PUBKEY);
    expect(mock.sentAPDUs[0].p1).toBe(P1.NON_CONFIRM);
  });

  it("getAddress with display sends P1.CONFIRM", async () => {
    const mock = new MockTransport();
    mock.pushResponse(Buffer.alloc(32, 0xcd));

    const xeris = new XerisLedger(mock as any);
    await xeris.getAddress("44'/9000'/0'/0'", true);

    expect(mock.sentAPDUs[0].p1).toBe(P1.CONFIRM);
  });

  it("signTransaction sends chunked data", async () => {
    const mock = new MockTransport();
    const signature = Buffer.alloc(64, 0xee);

    // The message is small enough for a single chunk (path + data < 255)
    mock.pushResponse(signature);

    const xeris = new XerisLedger(mock as any);
    const message = Buffer.alloc(100, 0x42);
    const result = await xeris.signTransaction("44'/9000'/0'/0'", message);

    expect(result.signature.equals(signature)).toBe(true);
    expect(mock.sentAPDUs[0].ins).toBe(INS.SIGN_TX);
  });

  it("signTransaction splits large payloads into chunks", async () => {
    const mock = new MockTransport();

    // Path is 1 + 4*4 = 17 bytes, so first chunk holds 255 - 17 = 238 bytes
    // A 500-byte message needs: 238 (first) + 255 (second) + 7 (third)
    mock.pushResponse(Buffer.alloc(0)); // response to first chunk
    mock.pushResponse(Buffer.alloc(0)); // response to second chunk
    mock.pushResponse(Buffer.alloc(64, 0xff)); // final response with signature

    const xeris = new XerisLedger(mock as any);
    const message = Buffer.alloc(500, 0xab);
    const result = await xeris.signTransaction("44'/9000'/0'/0'", message);

    expect(mock.sentAPDUs.length).toBe(3);
    expect(result.signature.length).toBe(64);

    // First chunk should have path + first slice
    const firstData = mock.sentAPDUs[0].data!;
    expect(firstData.length).toBe(255);
    expect(firstData[0]).toBe(4); // 4 path components

    // Continuation chunks use P1.MORE
    expect(mock.sentAPDUs[1].p1).toBe(P1.MORE);
    expect(mock.sentAPDUs[2].p1).toBe(P1.MORE);
  });

  it("signMessage uses correct instruction", async () => {
    const mock = new MockTransport();
    mock.pushResponse(Buffer.alloc(64, 0xdd));

    const xeris = new XerisLedger(mock as any);
    await xeris.signMessage("44'/9000'/0'/0'", Buffer.from("hello xeris"));

    expect(mock.sentAPDUs[0].ins).toBe(INS.SIGN_MESSAGE);
  });
});

// --- helpers ---

function readCompactU16(buf: Buffer, offset: number): [number, number] {
  let value = 0;
  let bytesRead = 0;
  for (let shift = 0; shift < 21; shift += 7) {
    const byte = buf[offset + bytesRead];
    bytesRead++;
    value |= (byte & 0x7f) << shift;
    if ((byte & 0x80) === 0) break;
  }
  return [value, bytesRead];
}
