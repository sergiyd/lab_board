export class BoardMessage {
  constructor(
    public readonly command: number,
    public readonly data?: ArrayBuffer
  ) {}
}
