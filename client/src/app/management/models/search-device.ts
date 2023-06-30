import { DeviceType } from "./device-type.enum";
import { DeviceBus } from "./device-bus.enum";

export class SearchDevice {
  constructor(
    public readonly index: number,
    public readonly bus: DeviceBus,
    public readonly type: DeviceType,
    public readonly typeName: string,
    public readonly registered: boolean,
    public readonly address: readonly number[]
  ) {}
}
