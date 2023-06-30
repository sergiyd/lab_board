import { Component, OnDestroy, OnInit } from "@angular/core";
import { Subject } from "rxjs";
import { takeUntil } from "rxjs/operators";
import { BoardServiceStatus } from "./models/board-service-status.enum";
import { BoardMode } from "./models/board-mode.enum";
import { BoardStateService } from "./services/board-state.service";
import { BoardService } from "./services/board.service";

@Component({
  selector: "app-root",
  templateUrl: "./app.component.html",
  styleUrls: ["./app.component.css"],
})
export class AppComponent implements OnInit, OnDestroy {
  public readonly title = "LAB Board";
  private readonly _unsubscribe = new Subject();
  private _status: BoardServiceStatus;
  public boardConnectionStatus: string;
  public connectionButtonText: string;
  private _capturing = false;
  public captureButtonText: string;

  public boardStateDescription: string;

  constructor(
    private readonly _boadService: BoardService,
    private readonly _boardStateService: BoardStateService
  ) {}

  public ngOnInit() {
    this._boadService.status$
      .pipe(takeUntil(this._unsubscribe))
      .subscribe((value) => {
        this._status = value;
        this.boardConnectionStatus = this._status.toString();

        switch (this._status) {
          case BoardServiceStatus.Connecting:
          case BoardServiceStatus.Reconnecting:
          case BoardServiceStatus.Online:
            this.connectionButtonText = "Disconnect";
            break;
          default:
            this.connectionButtonText = "Connect";
            break;
        }
      });
    this._boardStateService.state$
      .pipe(takeUntil(this._unsubscribe))
      .subscribe((value) => {
        this.boardStateDescription = BoardMode[value.mode];
        this._capturing = value.mode === BoardMode.Capturing;
        this.captureButtonText = this._capturing ? "Stop" : "Capture";
      });
  }

  public ngOnDestroy() {
    this._unsubscribe.next();
    this._unsubscribe.complete();
  }

  public connectionButtonClick() {
    // tslint:disable-next-line: no-bitwise
    ~[
      BoardServiceStatus.Connecting,
      BoardServiceStatus.Reconnecting,
      BoardServiceStatus.Online,
    ].indexOf(this._status)
      ? this._boadService.disconnect()
      : this._boadService.connect();
  }

  public captureButtonClick() {
    this._boardStateService.capture(!this._capturing);
  }

  public get online(): boolean {
    return this._status === BoardServiceStatus.Online;
  }
}
