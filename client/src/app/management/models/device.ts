import { DeviceType } from './device-type.enum';
import { DeviceDirection } from './device-direction.enum';
import { DeviceBus } from './device-bus.enum';

export class Device {
	constructor(public readonly index: number,
		public readonly name: string,
		public readonly bus: DeviceBus,
		public readonly busName: string,
		public readonly type: DeviceType,
		public readonly typeName: string,
		public readonly system: boolean,
		public readonly direction: DeviceDirection,
		public readonly enabled: boolean,
		public readonly address: ReadonlyArray<number>) { }
}
