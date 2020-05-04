import { Injectable } from '@angular/core';
import { BehaviorSubject, interval, Observable, Subject } from 'rxjs';
import { distinctUntilChanged, filter, map, share, take, takeUntil } from 'rxjs/operators';
import { WebSocketSubject, WebSocketSubjectConfig } from 'rxjs/webSocket';
import { BoardMessage } from '../models/board-message';
import { BoardServiceStatus } from '../models/board-service-status.enum';
import { ArrayUtilsService } from './array-utils.service';
import { ConfigResolverService } from './config-resolver.service';

@Injectable({
	providedIn: 'root'
})
export class BoardService {
	public static readonly commonSubject = 255;
	private readonly _reconnectTimeoutMs: number;
	private readonly _reconnectAttempts: number;

	private readonly _status: BehaviorSubject<BoardServiceStatus> =
		new BehaviorSubject<BoardServiceStatus>(BoardServiceStatus.Offline);

	private readonly _config: WebSocketSubjectConfig<SubjectMessage>;
	private readonly _messageSubject: Subject<SubjectMessage> = new Subject<SubjectMessage>();
	private _webSocketSubject: WebSocketSubject<SubjectMessage>;

	constructor(configResolverService: ConfigResolverService) {
		const config = configResolverService.resolve();
		this._reconnectTimeoutMs = config.reconnectTimeoutMs;
		this._reconnectAttempts = config.reconnectAttempts;

		this._config = {
			url: config.url,
			binaryType: 'arraybuffer',
			serializer: (message: SubjectMessage) => {
				const boardMessageBytes = Array.from(new Int8Array(ArrayUtilsService.boardMessageToBuffer(message.message)));
				// tslint:disable-next-line:no-bitwise
				return new Uint8Array([message.subject & 0xFF].concat(boardMessageBytes));
			},
			deserializer: (event: MessageEvent) => {
				return new SubjectMessage(new Uint8Array(event.data.slice(0, 1))[0],
					ArrayUtilsService.bufferToBoardMessage(event.data.slice(1)));
			},
			closeObserver: {
				next: () => {
					this._webSocketSubject = null;
					if (this._status.value !== BoardServiceStatus.Reconnecting) {
						this._status.next(BoardServiceStatus.Offline);
					}
				}
			},
			openObserver: {
				next: () => {
					this._status.next(BoardServiceStatus.Online);
				}
			}
		};
	}

	public connect(): void {
		this._status.next(BoardServiceStatus.Connecting);

		this._webSocketSubject = new WebSocketSubject<SubjectMessage>(this._config);
		this
			._webSocketSubject
			.subscribe(message => this._messageSubject.next(message), error => console.log(error));
	}

	public reconnect(): void {
		if (this._webSocketSubject) {
			this.disconnect();
		}

		this._status.next(BoardServiceStatus.Reconnecting)
		interval(this._reconnectTimeoutMs)
			.pipe(takeUntil(this._status.pipe(filter(status => status === BoardServiceStatus.Online))), take(this._reconnectAttempts))
			.subscribe(() => {
				this.connect();
			});
	}

	public send(subject: number, message: BoardMessage): void {
		this._webSocketSubject.next(new SubjectMessage(subject, message));
	}

	public disconnect(): void {
		if (this._webSocketSubject) {
			this._webSocketSubject.complete();
			this._webSocketSubject = null;
		}

		this._status.next(BoardServiceStatus.Offline);
	}

	public get status$(): Observable<BoardServiceStatus> {
		return this._status.pipe(share(), distinctUntilChanged());
	}

	public queue$(subject: number): Observable<BoardMessage> {
		return this._messageSubject
			.pipe(filter(m => m.subject === subject), map(m => m.message));
	}
}

class SubjectMessage {
	constructor(public readonly subject: number, public readonly message: BoardMessage) { }
}
