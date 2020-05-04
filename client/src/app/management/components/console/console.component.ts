import { Component } from '@angular/core';
import { Observable } from 'rxjs';
import { ManagementRegisteredDevice } from 'src/app/management/models/management-registered-device';
import { ManagementConsoleService } from 'src/app/management/services/management-console.service';

@Component({
	selector: 'app-console',
	templateUrl: './console.component.html',
	styleUrls: ['./console.component.css']
})
export class ManagementConsoleComponent {
	constructor(private readonly _managementConsoleService: ManagementConsoleService) { }

	public get devices$(): Observable<ReadonlyArray<ManagementRegisteredDevice>> {
		return this._managementConsoleService.registeredDevices$;
	}

	public switchDevice(index: number, turnOn: boolean): void {
		if (turnOn) {
			this._managementConsoleService.turnOnDevice(index);
		} else {
			this._managementConsoleService.turnOffDevice(index);
		}
	}

	public unregisterDevice(index: number): void {
		this._managementConsoleService.unregisterDevice(index);
	}

	public get idleMode$(): Observable<boolean> {
		return this._managementConsoleService.idleMode$;
	}
}
