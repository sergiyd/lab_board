import { SearchDevice } from './search-device';
import { DeviceType } from './device-type.enum';
import { ArrayUtilsService } from '../../services/array-utils.service';

export class ManagementSearchDevice {
	constructor(public readonly index: number, private readonly _device?: SearchDevice) { }

	public get loading(): boolean {
		return !this._device;
	}

	public get type(): DeviceType {
		return this._device.type;
	}

	public get typeName(): string {
		return this._device.typeName;
	}

	public get registered(): boolean {
		return this._device.registered;
	}

	public get canRegister(): boolean {
		return this.type !== DeviceType.Unknown && !this._device.registered;
	}

	public get addressHex(): string {
		return ArrayUtilsService.addressToHex(this._device.address);
	}

	public get device(): SearchDevice {
		return this._device;
	}
}
