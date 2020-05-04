import { ArrayUtilsService } from 'src/app/services/array-utils.service';
import { Device } from '../models/device';
import { DeviceBus } from '../models/device-bus.enum';
import { DeviceDirection } from '../models/device-direction.enum';
import { DeviceType } from '../models/device-type.enum';

export class DeviceDecoratorService {

	public static readonly buses: ReadonlyArray<DeviceBus> = Object.keys(DeviceBus).map(k => Number(k)).filter(v => !isNaN(v));

	public static readonly busNames: ReadonlyMap<DeviceBus, string> = new Map([
		[DeviceBus.Unknown, 'Unknown'],
		[DeviceBus.Analog, 'Analog'],
		[DeviceBus.Digital, 'Digital'],
		[DeviceBus.I2C, 'I2C'],
		[DeviceBus.OneWire, 'OneWire'],
		[DeviceBus.ExtensionDeviceAnalog, 'Ext. Analog'],
		[DeviceBus.ExtensionDeviceDigital, 'Ext. Digital']
	]);

	public static readonly types: ReadonlyArray<DeviceType> = Object.keys(DeviceType).map(k => Number(k)).filter(v => !isNaN(v));

	public static readonly typeNames: ReadonlyMap<DeviceType, string> = new Map([
		[DeviceType.Unknown, 'Unknown'],
		[DeviceType.Pin, 'Pin'],
		[DeviceType.Ds3231Time, 'DS3231 time'],
		[DeviceType.Ds3231Temperature, 'DS3231 temperature'],
		[DeviceType.Ds18B20, 'DS18B20'],
		[DeviceType.InterruptChange, 'Interrupt (change)'],
		[DeviceType.InterruptRising, 'Interrupt (rising)'],
		[DeviceType.InterruptFalling, 'Interrupt (falling)'],
		[DeviceType.ExtensionDevice, 'Extension device']
	]);

	constructor() { }

	public static decorateDevice(index: number,
		name: string,
		bus: number,
		type: number,
		flags: number,
		address: ArrayBuffer): Device {

		return new Device(index,
			name,
			bus,
			this.busNames.get(bus),
			type,
			DeviceDecoratorService.typeNames.get(type),
			// tslint:disable-next-line: no-bitwise
			!!(flags & DeviceFlags.System),
			// tslint:disable-next-line: no-bitwise
			flags & DeviceFlags.Direction ?
				DeviceDirection.Output :
				DeviceDirection.Input,
			// tslint:disable-next-line: no-bitwise
			!!(flags & DeviceFlags.Enabled),
			ArrayUtilsService.bufferToArray(address));
	}

	public static getDeviceFlags(device: Device): number {
		// tslint:disable-next-line: no-bitwise
		return (device.system ? DeviceFlags.System : 0) |
			(device.direction === DeviceDirection.Output ? DeviceFlags.Direction : 0) |
			(device.enabled ? DeviceFlags.Enabled : 0);
	}

	public static setEnabledFlag(device: Device, value: boolean): Device {
		return new Device(device.index,
			device.name,
			device.bus,
			device.busName,
			device.type,
			device.typeName,
			device.system,
			device.direction,
			value,
			device.address);
	}
}

class DeviceFlags {
	public static readonly System: number = 1;
	public static readonly Direction: number = 2;
	public static readonly Enabled: number = 4;
}
