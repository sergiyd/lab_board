import { Injectable } from '@angular/core';
import { BehaviorSubject, defer, Observable } from 'rxjs';
import { finalize } from 'rxjs/operators';
import { BufferReader } from 'src/app/models/buffer-reader';
import { BoardMessage } from '../../models/board-message';
import { ArrayUtilsService } from '../../services/array-utils.service';
import { BoardMessagingBaseService } from '../../services/board-messaging-base.service';
import { BoardStateService } from '../../services/board-state.service';
import { BoardService } from '../../services/board.service';
import { Device } from '../models/device';
import { DeviceBus } from '../models/device-bus.enum';
import { ManagementRegisteredDevice } from '../models/management-registered-device';
import { ManagementSearchDevice } from '../models/management-search-device';
import { SearchDevice } from '../models/search-device';
import { DeviceDecoratorService } from './device-decorator.service';

@Injectable({
	providedIn: 'root'
})
export class ManagementConsoleService extends BoardMessagingBaseService {
	private static readonly subject = 1;
	private _subscribed = false;
	private _idleMode = new BehaviorSubject<boolean>(false);
	private readonly _registeredDevicesSubject = new BehaviorSubject<Array<ManagementRegisteredDevice>>([]);
	private readonly _searchDevicesSubject = new BehaviorSubject<Array<ManagementSearchDevice>>([]);
	private readonly _registeredDevicesSubject$ = defer(() => {
		if (!this._subscribed) {
			this._subscribed = true;
			if (this._idleMode.value) {
				this.sendRegisteredDevicesCount();
			}
		}

		return this._registeredDevicesSubject as Observable<ReadonlyArray<ManagementRegisteredDevice>>;
	}).pipe(finalize(() => {
		this._subscribed = false;
		this._boardStateService.unsubscribeSubject(this.subject);
	}));

	constructor(boardService: BoardService,
		private readonly _boardStateService: BoardStateService) {
		super(boardService);

		this.subscribeOnQueue();

		this._boardStateService.idleMode$.subscribe(isIdle => {
			if (isIdle && this._subscribed) {
				this.sendRegisteredDevicesCount();
			}

			this._idleMode.next(isIdle);
		});
	}

	protected get subject(): number {
		return ManagementConsoleService.subject;
	}

	private sendRegisteredDevicesCount(): void {
		this.sendCommand(ManagementCommands.RegisteredDevicesCount);
	}

	private sendRegisteredDevice(index: number): void {
		this.sendCommand(ManagementCommands.RegisteredDevice, new Uint8Array([index]));
	}

	private sendUnregisterDevice(index: number): void {
		this.sendCommand(ManagementCommands.UnregisterDevice, new Uint8Array([index]));
	}

	private sendAllDevicesCount(bus: DeviceBus): void {
		this.sendCommand(ManagementCommands.AllDevicesCount, new Uint8Array([bus.valueOf()]));
	}

	private sendDevice(bus: DeviceBus, index: number): void {
		this.sendCommand(ManagementCommands.Device, new Uint8Array([bus.valueOf(), index]));
	}

	private sendSwitchDevice(index: number, value: boolean): void {
		this.sendCommand(ManagementCommands.SwitchDevice, new Uint8Array([index, value ? 1 : 0]));
	}

	private sendRegisterDevice(device: Device): void {
		const deviceData = [
			DeviceDecoratorService.getDeviceFlags(device),
			device.bus.valueOf(),
			device.type.valueOf(),
		].concat(ArrayUtilsService.stringToArray(device.name))
			.concat(device.address);

		this.sendCommand(ManagementCommands.RegisterDevice, new Uint8Array(deviceData));
	}

	protected processMessage(boardMessage: BoardMessage): void {
		switch (boardMessage.command as ManagementCommands) {
			case ManagementCommands.RegisteredDevicesCount:
				this.processRegisteredDevicesCount(boardMessage.data);
				break;
			case ManagementCommands.RegisteredDevice:
				this.processRegisteredDevice(boardMessage.data);
				break;
			case ManagementCommands.SwitchDevice:
				this.processSwitchDevice(boardMessage.data);
				break;
			case ManagementCommands.AllDevicesCount:
				this.processAllDevicesCount(boardMessage.data);
				break;
			case ManagementCommands.Device:
				this.processDevice(boardMessage.data);
				break;
		}
	}

	private processRegisteredDevicesCount(messageData: ArrayBuffer): void {
		const count = new BufferReader(messageData).readUint8();

		const devices = new Array<ManagementRegisteredDevice>(count);
		for (let deviceIndex = 0; deviceIndex < count; deviceIndex++) {
			this.sendRegisteredDevice(deviceIndex);
			devices[deviceIndex] = new ManagementRegisteredDevice(deviceIndex);
		}
		this._registeredDevicesSubject.next(devices);
	}

	private processRegisteredDevice(messageData: ArrayBuffer): void {
		const reader = new BufferReader(messageData);
		const index = reader.readUint8();
		const flags = reader.readUint8();
		const bus = reader.readUint8();
		const type = reader.readUint8();

		const device = DeviceDecoratorService.decorateDevice(index,
			ArrayUtilsService.bufferToString(reader.sliceSized()),
			bus,
			type,
			flags,
			reader.sliceSized());

		this._registeredDevicesSubject.value[index] = new ManagementRegisteredDevice(index, device);
	}

	private processSwitchDevice(messageData: ArrayBuffer): void {
		const reader = new BufferReader(messageData);
		const index = reader.readUint8();
		const isEnabled = !!reader.readUint8();

		const device = this._registeredDevicesSubject.value[index];

		this._registeredDevicesSubject.value[index] =
			new ManagementRegisteredDevice(index, DeviceDecoratorService.setEnabledFlag(device.device, isEnabled));
	}

	private processDevice(messageData: ArrayBuffer): void {
		const reader = new BufferReader(messageData);
		const bus = reader.readUint8();
		const index = reader.readUint8();
		const isRegistered = !!reader.readUint8();
		const type = reader.readUint8();

		const device = new SearchDevice(index,
			bus,
			type,
			DeviceDecoratorService.typeNames.get(type),
			isRegistered,
			ArrayUtilsService.bufferToArray(reader.sliceSized()));

		this._searchDevicesSubject.value[index] = new ManagementSearchDevice(device.index, device);
	}

	private processAllDevicesCount(messageData: ArrayBuffer): void {
		const reader = new BufferReader(messageData);
		const searchBus = reader.readUint8();
		const searchDeviceCount = reader.readUint8();

		const searchDevices = new Array<ManagementSearchDevice>(searchDeviceCount);
		for (let deviceIndex = 0; deviceIndex < searchDeviceCount; deviceIndex++) {
			this.sendDevice(searchBus, deviceIndex);
			searchDevices[deviceIndex] = new ManagementSearchDevice(deviceIndex);
		}
		this._searchDevicesSubject.next(searchDevices);
	}

	public get registeredDevices$(): Observable<ReadonlyArray<ManagementRegisteredDevice>> {
		return this._registeredDevicesSubject$;
	}

	public get foundDevices$(): Observable<ReadonlyArray<ManagementSearchDevice>> {
		return this._searchDevicesSubject;
	}

	public searchDevices(bus: DeviceBus): void {
		this.sendAllDevicesCount(bus);
	}

	public registerDevice(device: Device): void {
		this.sendRegisterDevice(device);
	}

	public unregisterDevice(index: number): void {
		this.sendUnregisterDevice(index);
	}

	public turnOnDevice(index: number): void {
		this.sendSwitchDevice(index, true);
	}

	public turnOffDevice(index: number): void {
		this.sendSwitchDevice(index, false);
	}

	public get idleMode$(): Observable<boolean> {
		return this._idleMode;
	}
}

enum ManagementCommands {
	RegisteredDevicesCount = 1,
	RegisteredDevice = 2,
	SwitchDevice = 3,
	RegisterDevice = 4,
	RenameDevice = 5,
	UnregisterDevice = 6,
	AllDevicesCount = 7,
	Device = 8
}
