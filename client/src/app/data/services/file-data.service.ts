import { Injectable } from '@angular/core';
import { BehaviorSubject, defer, Observable } from 'rxjs';
import { finalize } from 'rxjs/operators';
import { BoardMessage } from 'src/app/models/board-message';
import { ArrayUtilsService } from 'src/app/services/array-utils.service';
import { BoardMessagingBaseService } from 'src/app/services/board-messaging-base.service';
import { BoardStateService } from 'src/app/services/board-state.service';
import { BoardService } from 'src/app/services/board.service';
import { DataFile } from '../models/data-file';
import { BufferReader } from 'src/app/models/buffer-reader';

@Injectable({
	providedIn: 'root'
})
export class FileDataService extends BoardMessagingBaseService {
	private static readonly subject = 3;
	private readonly _files = new BehaviorSubject<Array<DataFile>>([]);
	private _idleMode = new BehaviorSubject<boolean>(false);
	private _subscribed = false;
	private readonly _files$ = defer(() => {
		if (!this._subscribed) {
			this._subscribed = true;
			if (this._idleMode.value) {
				this.sendFilesCount();
			}
		}

		return this._files as Observable<ReadonlyArray<DataFile>>;
	}).pipe(finalize(() => {
		this._subscribed = false;
		this._boardStateService.unsubscribeSubject(this.subject);
	}));

	constructor(private readonly _boardStateService: BoardStateService,
		boardService: BoardService) {

		super(boardService);

		this.subscribeOnQueue();

		this._boardStateService.idleMode$.subscribe(isIdle => {
			if (isIdle && this._subscribed) {
				this.sendFilesCount();
			}

			this._idleMode.next(isIdle);
		});
	}

	public get files$(): Observable<ReadonlyArray<DataFile>> {
		return this._files$;
	}

	public get idleMode$(): Observable<boolean> {
		return this._idleMode;
	}

	public removeFile(name: string) {
		this.sendRemoveFile(name);
	}

	protected get subject(): number {
		return FileDataService.subject;
	}

	private sendFilesCount(): void {
		this.sendCommand(HistoryCommands.FilesCount, new Uint8Array());
	}

	private sendFile(index: number): void {
		this.sendCommand(HistoryCommands.File, new Uint8Array([index]));
	}

	private sendRemoveFile(name: string): void {
		this.sendCommand(HistoryCommands.RemoveFile, new Uint8Array(ArrayUtilsService.stringToArray(name)));
	}

	protected processMessage(boardMessage: BoardMessage): void {
		switch (boardMessage.command as HistoryCommands) {
			case HistoryCommands.FilesCount:
				this.processFilesCount(boardMessage.data);
				break;
			case HistoryCommands.File:
				this.processFile(boardMessage.data);
				break;
			case HistoryCommands.RemoveFile:
				this.processRemoveFile(boardMessage.data);
				break;
		}
	}

	private processFilesCount(messageData: ArrayBuffer): void {
		const filesCount = new BufferReader(messageData).readUint8();

		this._files.next(new Array(filesCount).fill(0).map((v, i) => new DataFile(i, undefined, undefined)));

		for (let index = 0; index < filesCount; index++) {
			this.sendFile(index);
		}
	}

	private processFile(messageData: ArrayBuffer): void {
		const reader = new BufferReader(messageData);
		const index = reader.readUint8();
		const size = reader.readUint32();

		this._files.value[index] = new DataFile(index,
			ArrayUtilsService.bufferToString(reader.sliceSized()),
			size);
	}

	private processRemoveFile(messageData: ArrayBuffer): void {
		const fileName = ArrayUtilsService.bufferToString(new BufferReader(messageData).sliceSized());

		this._files.next(this._files.value.filter(f => f.name !== fileName));
	}
}

enum HistoryCommands {
	FilesCount = 1,
	File = 2,
	RemoveFile = 3
}
