import { Injectable } from '@angular/core';
import { BehaviorSubject, Observable } from 'rxjs';
import { distinctUntilChanged } from 'rxjs/operators';
import { BoardMessage } from '../models/board-message';
import { BoardMode } from '../models/board-mode.enum';
import { BoardServiceStatus } from '../models/board-service-status.enum';
import { BoardState } from '../models/board-state';
import { BufferReader } from '../models/buffer-reader';
import { BoardMessagingBaseService } from './board-messaging-base.service';
import { BoardService } from './board.service';

@Injectable({
	providedIn: 'root'
})
export class BoardStateService extends BoardMessagingBaseService {
	private static readonly subject: number = 0;
	private readonly _state = new BehaviorSubject<BoardState>(BoardState.unknown);

	private readonly _idleMode = new BehaviorSubject<boolean>(false);

	constructor(boardService: BoardService) {
		super(boardService);

		this.subscribeOnQueue();

		this._state.subscribe(state => {
			this._idleMode.next(state.mode === BoardMode.Idle);
		});

		this.boardService
			.status$
			.subscribe(status => {
				switch (status) {
					case BoardServiceStatus.Online:
						this.sendState();
						return;
					default:
						this._state.next(BoardState.unknown);
						return;
				}
			});
	}

	protected get subject(): number {
		return BoardStateService.subject;
	}

	public get state$(): Observable<BoardState> {
		return this._state;
	}

	public get idleMode$(): Observable<boolean> {
		return this._idleMode.pipe(distinctUntilChanged());
	}

	public subscribeSubject(subject: number): void {
		this.sendSubscribe(subject, true);
	}

	public unsubscribeSubject(subject: number): void {
		this.sendSubscribe(subject, false);
	}

	protected processMessage(boardMessage: BoardMessage): void {
		switch (boardMessage.command as BoardCommands) {
			case BoardCommands.State:
				const reader = new BufferReader(boardMessage.data);
				const mode = reader.readUint8() as BoardMode;
				const syncInterval = reader.readUint8();
				const modeTimestamp = reader.readUint32();
				this._state.next(new BoardState(mode, syncInterval, modeTimestamp));
				if (mode === BoardMode.Restarting) {
					this.boardService.reconnect();
				}
				break;
		}
	}

	private sendState(): void {
		this.sendCommand(BoardCommands.State);
	}

	public capture(active: boolean): void {
		this.sendCommand(BoardCommands.Capture, new Uint8Array([+active]));
	}

	private sendSubscribe(subject: number, value: boolean): void {
		this.sendCommand(BoardCommands.Subscribe, new Uint8Array([subject, +value]));
	}
}

enum BoardCommands {
	State = 1,
	Capture = 2,
	Subscribe = 3
}
