import { BoardMessage } from "../models/board-message";

export class ArrayUtilsService {
  constructor() {}

  public static bufferToBoardMessage(buffer: ArrayBuffer): BoardMessage {
    // tslint:disable-next-line: no-bitwise
    return new BoardMessage(
      new Uint8Array(buffer.slice(0, 2)).reduce(
        (r, c, i) => (r += c << (8 * i)),
        0
      ),
      buffer.slice(2)
    );
  }

  public static boardMessageToBuffer(boardMessage: BoardMessage): ArrayBuffer {
    // tslint:disable-next-line: no-bitwise
    const commandBytes = Array.from(
      new Uint8Array([boardMessage.command & 0xff, boardMessage.command >> 8])
    );
    return new Uint8Array(
      boardMessage.data
        ? commandBytes.concat(this.bufferToArray(boardMessage.data))
        : commandBytes
    );
  }

  public static addressToHex(address: readonly number[]): string {
    return address
      .map((b) => b.toString(16))
      .join(":")
      .toUpperCase();
  }

  public static hexToAddress(addressHex: string): readonly number[] {
    return addressHex.split(":").map((hv) => Number.parseInt(hv, 16));
  }

  public static bufferToString(buffer: ArrayBuffer): string {
    return this.bufferToArray(buffer).reduce(
      (p, c) => p + String.fromCharCode(c),
      ""
    );
  }

  public static bufferToArray(buffer: ArrayBuffer): readonly number[] {
    return Array.from(new Uint8Array(buffer));
  }

  public static stringToArray(value: string): readonly number[] {
    const result = new Array(value.length + 1);
    result[0] = value.length;
    for (let charIndex = 0; charIndex < value.length; charIndex++) {
      result[charIndex + 1] = value.charCodeAt(charIndex);
    }
    return result;
  }
}
