import { DeviceType } from './device-type.enum';

export class DeviceTypeMenuItem {
	constructor(public readonly type: DeviceType,
		public readonly name: string) { }
}
