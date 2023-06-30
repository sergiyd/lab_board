import { Observable } from "rxjs";
import { ConvertionTypes } from "../../models/convertion-types.enum";
import { FormatterCodes } from "../../models/formatter-codes.enum";
import { Source } from "../../models/source";
import { SourceData } from "../../models/source-data";
import { map } from "rxjs/operators";

export class Dataset {
  private readonly _data$ = this.sourceData$.pipe(map((v) => v.data));

  public color: string;
  public convertionType: ConvertionTypes;
  public formatterCode: FormatterCodes;
  public unmuted: boolean;

  constructor(
    private readonly _source: Source,
    public readonly sourceData$: Observable<SourceData>
  ) {
    const extra = new Uint8Array(_source.extra);
    this.color = `#${Dataset.toHex(extra[0])}${Dataset.toHex(
      extra[1]
    )}${Dataset.toHex(extra[2])}`;
    this.convertionType = extra[3];
    this.formatterCode = extra[4];
    this.unmuted = !_source.muted;
  }

  public get index(): number {
    return this._source.index;
  }

  public get name(): string {
    return this._source.name;
  }

  public get extra(): readonly number[] {
    return [
      this.colorRed,
      this.colorGreen,
      this.colorBlue,
      this.convertionType,
      this.formatterCode,
    ];
  }

  public get colorRed(): number {
    return Number.parseInt(this.color.substring(1, 3), 16);
  }

  public get colorGreen(): number {
    return Number.parseInt(this.color.substring(3, 5), 16);
  }

  public get colorBlue(): number {
    return Number.parseInt(this.color.substring(5, 7), 16);
  }

  private static toHex(value: number): string {
    return ("0" + value.toString(16)).slice(-2);
  }

  public get data$(): Observable<ArrayBuffer> {
    return this._data$;
  }
}
