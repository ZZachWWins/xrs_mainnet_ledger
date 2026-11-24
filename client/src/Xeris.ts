import Transport from "@ledgerhq/hw-transport";
import {
  CLA,
  CHUNK_SIZE,
  INS,
  P1,
  P2_DEFAULT,
  PUBKEY_LENGTH,
  SIGNATURE_LENGTH,
  StatusWord,
  type XerisAddress,
  type XerisSignature,
  type XerisVersion,
} from "./types";
import { base58Encode, serializePath } from "./utils";

/**
 * Xeris Ledger hardware wallet client.
 *
 * Communicates with a Xeris app running on a Ledger device over
 * any `@ledgerhq/hw-transport` implementation (USB, Bluetooth, etc.).
 *
 * @example
 * ```ts
 * import TransportNodeHid from "@ledgerhq/hw-transport-node-hid";
 * import { XerisLedger } from "@xeris/ledger-app";
 *
 * const transport = await TransportNodeHid.create();
 * const xeris = new XerisLedger(transport);
 *
 * const version = await xeris.getVersion();
 * const { address } = await xeris.getAddress("44'/9000'/0'/0'");
 * ```
 */
export class XerisLedger {
  private transport: Transport;

  constructor(transport: Transport) {
    this.transport = transport;
  }

  /**
   * Retrieve the app version installed on the Ledger device.
   *
   * @returns An object with `major`, `minor`, `patch` version numbers.
   */
  async getVersion(): Promise<XerisVersion> {
    const response = await this.send(
      INS.GET_VERSION,
      P1.NON_CONFIRM,
      P2_DEFAULT,
    );

    if (response.length < 3) {
      throw new Error(
        `Unexpected version response length: ${response.length}`,
      );
    }

    return {
      major: response[0],
      minor: response[1],
      patch: response[2],
    };
  }

  /**
   * Derive and return the public key (and base58 address) for a
   * given BIP-44 derivation path.
   *
   * @param path    BIP-44 path, e.g. `"44'/9000'/0'/0'"`.
   * @param display If `true`, the address is shown on the Ledger
   *                screen for user verification.
   */
  async getAddress(
    path: string,
    display = false,
  ): Promise<XerisAddress> {
    const pathBuf = serializePath(path);
    const p1 = display ? P1.CONFIRM : P1.NON_CONFIRM;

    const response = await this.send(
      INS.GET_PUBKEY,
      p1,
      P2_DEFAULT,
      pathBuf,
    );

    if (response.length < PUBKEY_LENGTH) {
      throw new Error(
        `Unexpected pubkey response length: ${response.length}`,
      );
    }

    const publicKey = response.subarray(0, PUBKEY_LENGTH);
    const address = base58Encode(publicKey);

    return { publicKey: Buffer.from(publicKey), address };
  }

  /**
   * Sign a serialized transaction message on the Ledger device.
   *
   * The `message` should be the output of
   * `XerisTransaction.serializeMessage()` — the unsigned transaction
   * message bytes that will be hashed and signed with Ed25519.
   *
   * The Ledger displays transaction details for user confirmation.
   *
   * @param path    BIP-44 derivation path of the signing key.
   * @param message Serialized transaction message bytes.
   * @returns A 64-byte Ed25519 signature.
   */
  async signTransaction(
    path: string,
    message: Buffer,
  ): Promise<XerisSignature> {
    const response = await this.sendChunked(INS.SIGN_TX, path, message);

    if (response.length < SIGNATURE_LENGTH) {
      throw new Error(
        `Unexpected signature response length: ${response.length}`,
      );
    }

    return {
      signature: Buffer.from(response.subarray(0, SIGNATURE_LENGTH)),
    };
  }

  /**
   * Sign an arbitrary off-chain message.
   *
   * The Ledger displays a "Sign Message" prompt with the message
   * content (or its hash for large messages).
   *
   * @param path    BIP-44 derivation path of the signing key.
   * @param message Arbitrary bytes to sign.
   * @returns A 64-byte Ed25519 signature.
   */
  async signMessage(
    path: string,
    message: Buffer,
  ): Promise<XerisSignature> {
    const response = await this.sendChunked(
      INS.SIGN_MESSAGE,
      path,
      message,
    );

    if (response.length < SIGNATURE_LENGTH) {
      throw new Error(
        `Unexpected signature response length: ${response.length}`,
      );
    }

    return {
      signature: Buffer.from(response.subarray(0, SIGNATURE_LENGTH)),
    };
  }

  /**
   * Send a payload to the device in multiple APDU chunks.
   *
   * The first chunk carries the BIP-44 path prepended to the first
   * slice of the payload. Subsequent chunks carry continuation data
   * with P1=0x80 (MORE).
   */
  private async sendChunked(
    ins: number,
    path: string,
    payload: Buffer,
  ): Promise<Buffer> {
    const pathBuf = serializePath(path);
    const header = pathBuf;

    const firstDataCapacity = CHUNK_SIZE - header.length;
    if (firstDataCapacity <= 0) {
      throw new Error("Derivation path too long for APDU frame");
    }

    const firstSlice = payload.subarray(
      0,
      Math.min(firstDataCapacity, payload.length),
    );
    const firstChunk = Buffer.concat([header, firstSlice]);
    let offset = firstSlice.length;

    const isLast = offset >= payload.length;
    let response = await this.send(
      ins,
      isLast ? P1.NON_CONFIRM : P1.NON_CONFIRM,
      P2_DEFAULT,
      firstChunk,
    );

    while (offset < payload.length) {
      const remaining = payload.length - offset;
      const chunkSize = Math.min(CHUNK_SIZE, remaining);
      const chunk = payload.subarray(offset, offset + chunkSize);
      offset += chunkSize;

      const p1 = offset >= payload.length ? P1.MORE : P1.MORE;
      response = await this.send(ins, p1, P2_DEFAULT, chunk);
    }

    return response;
  }

  /**
   * Send a single APDU to the device and return the response data.
   *
   * Throws a descriptive error if the status word indicates failure.
   */
  private async send(
    ins: number,
    p1: number,
    p2: number,
    data?: Buffer,
  ): Promise<Buffer> {
    const response = await this.transport.send(
      CLA,
      ins,
      p1,
      p2,
      data,
      [StatusWord.OK],
    );

    return response.subarray(0, response.length - 2);
  }
}
