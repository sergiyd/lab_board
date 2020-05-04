import { Component, OnDestroy, OnInit } from '@angular/core';
import { Observable } from 'rxjs';
import { DeviceBus } from 'src/app/management/models/device-bus.enum';
import { DeviceDirection } from 'src/app/management/models/device-direction.enum';
import { DeviceType } from 'src/app/management/models/device-type.enum';
import { DeviceDecoratorService } from 'src/app/management/services/device-decorator.service';
import { RegisterDeviceService } from 'src/app/management/services/register-device.service';
import { ArrayUtilsService } from 'src/app/services/array-utils.service';
import { DeviceBusMenuItem } from '../../models/device-bus-menu-item';
import { DeviceTypeMenuItem } from '../../models/device-type-menu-item';

@Component({
	selector: 'app-register-device',
	templateUrl: './register-device.component.html',
	styleUrls: ['./register-device.component.css']
})
export class RegisterDeviceComponent implements OnInit, OnDestroy {

	private static readonly buses: ReadonlyMap<DeviceBus, string> = new Map<DeviceBus, string>(
		RegisterDeviceService.buses.map(b => [b, DeviceDecoratorService.busNames.get(b)])
	);
	public static readonly busItems: ReadonlyArray<DeviceBusMenuItem> =
		Array.from(RegisterDeviceComponent.buses.keys()).map(k => new DeviceBusMenuItem(k, RegisterDeviceComponent.buses.get(k)));

	private static readonly types: ReadonlyMap<DeviceType, string> = new Map<DeviceType, string>(
		Array.from(DeviceDecoratorService.types).map(t => [t, DeviceDecoratorService.typeNames.get(t)])
	);
	private static typesItems: ReadonlyArray<DeviceTypeMenuItem> =
		Array.from(RegisterDeviceComponent.types.keys()).map(k => new DeviceTypeMenuItem(k, RegisterDeviceComponent.types.get(k)));

	public readonly directionInput: DeviceDirection = DeviceDirection.Input;
	public readonly directionOutput: DeviceDirection = DeviceDirection.Output;

	constructor(private readonly _registerDeviceService: RegisterDeviceService) { }

	ngOnInit() {
	}

	ngOnDestroy(): void {
		this._registerDeviceService.reset();
	}

	public get busItems(): ReadonlyArray<DeviceBusMenuItem> {
		return RegisterDeviceComponent.busItems;
	}

	public get typeItems(): ReadonlyArray<DeviceTypeMenuItem> {
		return RegisterDeviceComponent.typesItems;
	}

	public register(): void {
		this._registerDeviceService.register();
	}

	public get name(): string {
		return this._registerDeviceService.name;
	}
	public set name(value: string) {
		this._registerDeviceService.name = value;
	}

	public get bus(): DeviceBus {
		return this._registerDeviceService.bus;
	}
	public set bus(value: DeviceBus) {
		this._registerDeviceService.bus = value;
	}

	public get type(): DeviceType {
		return this._registerDeviceService.type;
	}
	public set type(value: DeviceType) {
		this._registerDeviceService.type = value;
	}

	public get direction(): DeviceDirection {
		return this._registerDeviceService.direction;
	}

	public set direction(value: DeviceDirection) {
		this._registerDeviceService.direction = value;
	}

	public get address(): string {
		return ArrayUtilsService.addressToHex(Array.from(new Uint8Array(this._registerDeviceService.address)));
	}

	public set address(value: string) {
		this._registerDeviceService.address = ArrayUtilsService.hexToAddress(value);
	}

	public get idleMode$(): Observable<boolean> {
		return this._registerDeviceService.idleMode$;
	}
}
