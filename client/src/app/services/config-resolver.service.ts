import { Inject, Injectable } from "@angular/core";
import { environment } from "src/environments/environment";
import { BoardServiceConfig } from "../models/board-service-config";
import { WINDOW } from "../providers/window-provider";

@Injectable({
  providedIn: "root",
})
export class ConfigResolverService {
  constructor(@Inject(WINDOW) private readonly _window: Window) {}

  public resolve(): BoardServiceConfig {
    const wsUrl = environment.production
      ? this.buildWsUrl(this._window.location.hostname)
      : this.buildWsUrl(environment.boardHost);

    const boardUrl = environment.production
      ? this.buildHttpUrl(this._window.location.hostname)
      : this.buildHttpUrl(environment.boardHost);

    return {
      wsUrl,
      boardUrl,
      reconnectTimeoutMs: environment.reconnectTimeoutMs,
      reconnectAttempts: environment.reconnectAttempts,
    };
  }

  private buildWsUrl(host: string): string {
    return `ws://${host}:81`;
  }

  private buildHttpUrl(host: string): string {
    return `http://${host}`;
  }
}
