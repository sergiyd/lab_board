import { SourceFlags } from "./source-flags.enum";

export class SourceData {
  public static readonly dataLength = 4;

  constructor(
    public readonly index: number,
    public readonly flags: SourceFlags,
    public readonly data: ArrayBuffer
  ) {}
}
