import { DeviceBus } from './device-bus.enum';

export class DeviceBusMenuItem {
	constructor(public readonly bus: DeviceBus,
		public readonly name: string) { }
}
