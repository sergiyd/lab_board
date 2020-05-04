import { Inject, Injectable } from '@angular/core';
import { environment } from 'src/environments/environment';
import { BoardServiceConfig } from '../models/board-service-config';
import { WINDOW } from '../providers/window-provider';

@Injectable({
	providedIn: 'root'
})
export class ConfigResolverService {

	constructor(@Inject(WINDOW) private readonly _window: Window) { }

	public resolve(): BoardServiceConfig {
		const configUrl = environment.production ?
			this.getRelativeUrl() :
			environment.wsUrl;

		return {
			url: configUrl,
			reconnectTimeoutMs: environment.reconnectTimeoutMs,
			reconnectAttempts: environment.reconnectAttempts
		}
	}

	private getRelativeUrl(): string {
		return `ws://${this._window.location.hostname}:81`;
	}
}

