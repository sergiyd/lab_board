import { Device } from './device';
import { DeviceDirection } from './device-direction.enum';

export class ManagementRegisteredDevice {
	constructor(public readonly index: number, private readonly _device?: Device) { }

	public get loading(): boolean {
		return !this._device;
	}

	public get name(): string {
		return this._device.name;
	}

	public get busName(): string {
		return this._device.busName;
	}

	public get typeName(): string {
		return this._device.typeName;
	}

	public get direction(): string {
		return DeviceDirection[this._device.direction];
	}

	public get enabled(): boolean {
		return this._device.enabled;
	}

	public get system(): boolean {
		return this._device.system;
	}

	public get address(): string {
		return this
			._device
			.address
			.reduce((p, c, i, a) => p += `${c.toString(16)}${i < a.length - 1 ? ' ' : ''}`, '');
	}

	public get device(): Device {
		return this._device;
	}
}
