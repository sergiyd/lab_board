import { Injectable } from '@angular/core';
import { Observable } from 'rxjs';
import { BoardStateService } from 'src/app/services/board-state.service';
import { Device } from '../models/device';
import { DeviceBus } from '../models/device-bus.enum';
import { DeviceDirection } from '../models/device-direction.enum';
import { DeviceType } from '../models/device-type.enum';
import { DeviceDecoratorService } from './device-decorator.service';
import { ManagementConsoleService } from './management-console.service';

@Injectable({
	providedIn: 'root'
})
export class RegisterDeviceService {
	public static readonly buses: readonly DeviceBus[] =
		Array.from(DeviceDecoratorService.buses.filter(b => b !== DeviceBus.Unknown));

	public name: string;
	public bus: DeviceBus;
	public type: DeviceType;
	public direction: DeviceDirection;
	public address: readonly number[];

	constructor(private readonly _boardStateService: BoardStateService,
		private readonly _managementConsoleService: ManagementConsoleService) {
		this.reset();
	}

	public populate(name: string,
		bus: DeviceBus,
		type: DeviceType,
		direction: DeviceDirection,
		address: readonly number[]): void {

		this.name = name;
		this.bus = bus;
		this.type = type;
		this.direction = direction;
		this.address = address;
	}

	public reset(): void {
		this.populate('', RegisterDeviceService.buses[0], DeviceType.Unknown, DeviceDirection.Input, []);
	}

	public register(): void {
		if (this.type === DeviceType.Unknown) {
			return;
		}

		const device = new Device(indexRegisterDefault,
			this.name,
			this.bus,
			busNameRegisterDefault,
			this.type,
			typeNameRegisterDefault,
			notSystem,
			this.direction,
			enabledRegisterDefault,
			mutedRegisterDefault,
			this.address);

		this._managementConsoleService.registerDevice(device);
	}

	public get idleMode$(): Observable<boolean> {
		return this._boardStateService.idleMode$;
	}
}

const indexRegisterDefault = 0;
const busNameRegisterDefault: string = undefined;
const typeNameRegisterDefault: string = undefined;
const notSystem = false;
const enabledRegisterDefault = true;
const mutedRegisterDefault = false;
