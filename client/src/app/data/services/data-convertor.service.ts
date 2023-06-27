import { ConvertionTypes } from '../models/convertion-types.enum';
import { Convertor } from '../models/convertor';
import { Formatter } from '../models/formatter';
import { FormatterCodes } from '../models/formatter-codes.enum';
import { PhysicalClassifier } from '../models/physical-classifier.enum';

/* Convertors */
abstract class ConvertorBase implements Convertor {
	constructor(public readonly type: ConvertionTypes,
		public readonly name: string) { }
	public abstract convert(data: DataView): number;
}

class IntegerConvertor extends ConvertorBase {
	constructor() {
		super(ConvertionTypes.Integer, 'Integer');
	}
	public convert(data: DataView): number {
		return data.getUint32(0, true);
	}
}

class FloatConvertor extends ConvertorBase {
	constructor() {
		super(ConvertionTypes.Float, 'Float');
	}
	public convert(data: DataView): number {
		return data.getInt32(0, true);
	}
}

class Ds18Convertor extends ConvertorBase {
	constructor() {
		super(ConvertionTypes.Ds18, 'DS18');
	}
	public convert(data: DataView): number {
		return data.getInt16(0, true) * 0.0078125;
	}
}

class SystemTemperatureConvertor extends ConvertorBase {
	constructor() {
		super(ConvertionTypes.SystemTemperature, 'Sys. Temp.');
	}
	public convert(data: DataView): number {
		return data.getInt16(0, true) / 100;
	}
}

class Ds3231Convertor extends ConvertorBase {
	constructor() {
		super(ConvertionTypes.Ds3231, 'DS3231');
	}
	public convert(data: DataView): number {
		return data.getInt16(0, true) / 100;
	}
}

abstract class Acs712AConvertorBase extends ConvertorBase {
	constructor(public readonly type: ConvertionTypes,
		public readonly name: string) {
		super(type, name);
	}

	protected abstract get coefficient(): number;

	public convert(data: DataView): number {
		const vLogic = 5;
		const vValue = 1023 / vLogic;
		return (data.getInt16(0, true) - (vValue * 2.5)) / (vValue * this.coefficient);
	}
}

class Acs712A05Convertor extends Acs712AConvertorBase {
	constructor() {
		super(ConvertionTypes.Acs712A05, 'ACS712 5A');
	}
	protected get coefficient(): number {
		return 0.185;
	}
}

class Acs712A20Convertor extends Acs712AConvertorBase {
	constructor() {
		super(ConvertionTypes.Acs712A20, 'ACS712 20A');
	}
	protected get coefficient(): number {
		return 0.1;
	}
}

class Acs712A30Convertor extends Acs712AConvertorBase {
	constructor() {
		super(ConvertionTypes.Acs712A30, 'ACS712 30A');
	}
	protected get coefficient(): number {
		return 0.066;
	}
}

/* Formatter */
abstract class FormatterBase implements Formatter {
	constructor(public readonly code: number,
		public readonly physicalClassifier: PhysicalClassifier,
		public readonly name: string,
		public readonly prefix?: string,
		public readonly suffix?: string) { }

	public format(value: number): string {
		return `${this.prefix || ''}${this.formatString(this.formatNumber(value))}${this.suffix || ''}`;
	}
	public abstract formatNumber(value: number): number;
	public abstract formatString(value: number): string;
}

class DecimalFormatter extends FormatterBase {
	constructor() {
		super(FormatterCodes.Decimal, PhysicalClassifier.Number, 'Decimal');
	}
	public formatNumber(value: number): number {
		return value;
	}
	public formatString(value: number): string {
		return value.toString();
	}
}

class HexadecimalFormatter extends FormatterBase {
	constructor() {
		super(FormatterCodes.Hexadecimal, PhysicalClassifier.Number, 'Hexadecimal', '0x');
	}
	public formatString(value: number): string {
		return value.toString(16).toUpperCase();
	}

	public formatNumber(value: number): number {
		return value;
	}
}

class CelsiusFormatter extends FormatterBase {
	constructor() {
		super(FormatterCodes.Celsius, PhysicalClassifier.Temperature, 'Celsius', '', '°C');
	}
	public formatString(value: number): string {
		return value.toString();
	}

	public formatNumber(value: number): number {
		return value;
	}
}

class FahrenheitFormatter extends FormatterBase {
	constructor() {
		super(FormatterCodes.Fahrenheit, PhysicalClassifier.Temperature, 'Fahrenheit', '', '°F');
	}
	public formatString(value: number): string {
		return value.toString();
	}

	public formatNumber(value: number): number {
		return (value * 9 / 5) + 32;
	}
}

class KelvinFormatter extends FormatterBase {
	constructor() {
		super(FormatterCodes.Kelvin, PhysicalClassifier.Temperature, 'Kelvin', '', 'K');
	}
	public formatString(value: number): string {
		return value.toString();
	}

	public formatNumber(value: number): number {
		return value + 273.15;
	}
}

export class DataConvertorService {
	private static readonly defaultConvertorType = ConvertionTypes.Integer;
	private static readonly defaultFormatterCode = FormatterCodes.Decimal;

	private static readonly formatters: ReadonlyMap<FormatterCodes, FormatterBase> = new Map<FormatterCodes, FormatterBase>([
		[FormatterCodes.Decimal, new DecimalFormatter()],
		[FormatterCodes.Hexadecimal, new HexadecimalFormatter()],
		[FormatterCodes.Celsius, new CelsiusFormatter()],
		[FormatterCodes.Fahrenheit, new FahrenheitFormatter()],
		[FormatterCodes.Kelvin, new KelvinFormatter()],
	]);

	private static readonly convertors: ReadonlyMap<ConvertionTypes, ConvertorBase> = new Map<ConvertionTypes, ConvertorBase>([
		[ConvertionTypes.Integer, new IntegerConvertor()],
		[ConvertionTypes.Float, new FloatConvertor()],
		[ConvertionTypes.Ds18, new Ds18Convertor()],
		[ConvertionTypes.SystemTemperature, new SystemTemperatureConvertor()],
		[ConvertionTypes.Acs712A05, new Acs712A05Convertor()],
		[ConvertionTypes.Acs712A20, new Acs712A20Convertor()],
		[ConvertionTypes.Acs712A30, new Acs712A30Convertor()],
		[ConvertionTypes.Ds3231, new Ds3231Convertor()]
	]);

	public static getFormatters(): ReadonlyMap<FormatterCodes, string> {
		return new Map(Array.from(DataConvertorService.formatters.values()).map(formatter => [formatter.code, formatter.name]));
	}

	public static getConvertors(): ReadonlyMap<ConvertionTypes, string> {
		return new Map(Array.from(DataConvertorService.convertors.values()).map(convertor => [convertor.type, convertor.name]));
	}

	public static convertNumber(data: DataView, convertionType: ConvertionTypes, formatterCode: FormatterCodes): number {
		if (!data) {
			return;
		}

		const convertor = DataConvertorService.convertors.get(convertionType || (DataConvertorService.defaultConvertorType));
		const formatter = DataConvertorService.formatters.get(formatterCode || (DataConvertorService.defaultFormatterCode));

		return formatter.formatNumber(convertor.convert(data));
	}

	public static format(data: DataView, convertionType: ConvertionTypes, formatterCode: FormatterCodes): string {
		const convertor = DataConvertorService.convertors.get(convertionType || (DataConvertorService.defaultConvertorType));
		const formatter = DataConvertorService.formatters.get(formatterCode || (DataConvertorService.defaultFormatterCode));

		return formatter.format(convertor.convert(data));
	}
}

