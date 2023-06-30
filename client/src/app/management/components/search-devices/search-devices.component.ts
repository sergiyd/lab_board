import { Component } from '@angular/core';
import { Observable } from 'rxjs';
import { DeviceBus } from 'src/app/management/models/device-bus.enum';
import { DeviceDirection } from 'src/app/management/models/device-direction.enum';
import { ManagementSearchDevice } from 'src/app/management/models/management-search-device';
import { DeviceDecoratorService } from 'src/app/management/services/device-decorator.service';
import { ManagementConsoleService } from 'src/app/management/services/management-console.service';
import { RegisterDeviceService } from 'src/app/management/services/register-device.service';
import { DeviceBusMenuItem } from '../../models/device-bus-menu-item';
import { RegisterDeviceComponent } from '../register-device/register-device.component';

@Component({
	selector: 'app-search-devices',
	templateUrl: './search-devices.component.html',
	styleUrls: ['./search-devices.component.css']
})
export class SearchDevicesComponent {
	public bus: DeviceBus = DeviceDecoratorService.buses.filter(b => b !== DeviceBus.Unknown)[0];

	constructor(private readonly _managementConsoleService: ManagementConsoleService,
		private readonly _registerDeviceService: RegisterDeviceService) { }

	public search(): void {
		this._managementConsoleService.searchDevices(this.bus);
	}

	public get devices$(): Observable<readonly ManagementSearchDevice[]> {
		return this._managementConsoleService.foundDevices$;
	}

	public populate(device: ManagementSearchDevice): void {
		this._registerDeviceService.populate(`${device.typeName} device #${device.index + 1}`,
			this.bus,
			device.type,
			DeviceDirection.Input,
			device.device.address);
	}

	public get busItems(): readonly DeviceBusMenuItem[] {
		return RegisterDeviceComponent.busItems;
	}

	public get idleMode$(): Observable<boolean> {
		return this._registerDeviceService.idleMode$;
	}
}
