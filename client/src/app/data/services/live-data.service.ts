import { Injectable } from '@angular/core';
import { BehaviorSubject, Observable, Subject } from 'rxjs';
import { BoardMessage } from 'src/app/models/board-message';
import { BoardMode } from 'src/app/models/board-mode.enum';
import { BufferReader } from 'src/app/models/buffer-reader';
import { ArrayUtilsService } from 'src/app/services/array-utils.service';
import { BoardMessagingBaseService } from 'src/app/services/board-messaging-base.service';
import { BoardStateService } from 'src/app/services/board-state.service';
import { BoardService } from 'src/app/services/board.service';
import { Source } from '../models/source';
import { SourceData } from '../models/source-data';
import { SourceFlags } from '../models/source-flags.enum';
import { SourcesSync } from '../models/sources-sync';

@Injectable({
	providedIn: 'root'
})
export class LiveDataService extends BoardMessagingBaseService {
	private static readonly subject: number = 2;
	private readonly _sourcesSubjects = new Subject<Array<Subject<Source>>>();
	private readonly _fetchingSubject = new Subject<boolean>();
	private readonly _sourcesSyncSubject = new Subject<SourcesSync>();
	private readonly _canFetchSubject = new BehaviorSubject<boolean>(false);
	private readonly _sourceRequests = new Map<number, Subject<Source>>();
	private _syncInterval: number;

	constructor(private readonly _boardStateService: BoardStateService,
		boardService: BoardService) {

		super(boardService);

		this._boardStateService.idleMode$.subscribe(this._canFetchSubject);
		this._boardStateService.state$.subscribe(state => {
			this._syncInterval = state.syncInterval;
			if (state.mode !== BoardMode.Idle && state.mode !== BoardMode.Capturing) {
				this._fetchingSubject.next(false);
			}
		});

		this.fetching$.subscribe(() => this._sourceRequests.clear());

		this.subscribeOnQueue();
	}

	protected get subject(): number {
		return LiveDataService.subject;
	}

	private sendSourcesCount(): void {
		this.sendCommand(MonitoringCommands.SourcesCount, new Uint8Array([+autoSubscribe]));
	}

	private sendSource(index: number): void {
		this.sendCommand(MonitoringCommands.Source, new Uint8Array([index]));
	}

	private sendSourceData(index: number): void {
		this.sendCommand(MonitoringCommands.SourceData, new Uint8Array([index]));
	}

	private sendPutExtra(index: number, extra: ReadonlyArray<number>): void {
		this.sendCommand(MonitoringCommands.PutExtra, new Uint8Array([index, extra.length].concat(extra)));
	}

	protected processMessage(boardMessage: BoardMessage): void {
		switch (boardMessage.command as MonitoringCommands) {
			case MonitoringCommands.SourcesCount:
				this.processSourcesCount(boardMessage.data);
				break;
			case MonitoringCommands.Source:
				this.processSource(boardMessage.data);
				break;
			case MonitoringCommands.SourceData:
				this.processSourceData(boardMessage.data);
				break;
		}
	}

	private processSourcesCount(messageData: ArrayBuffer): void {
		const sourceIndexes = new Uint8Array(new BufferReader(messageData).sliceSized());
		const sourcesSubjects = new Array<Subject<Source>>(sourceIndexes.length);

		for (let index = 0; index < sourceIndexes.length; index++) {
			const sourceRequest = new Subject<Source>();
			this._sourceRequests.set(sourceIndexes[index], sourceRequest);
			sourcesSubjects[index] = sourceRequest;
			this.sendSource(sourceIndexes[index]);
		}
		this._sourcesSubjects.next(sourcesSubjects);
	}

	private processSource(messageData: ArrayBuffer): void {
		const reader = new BufferReader(messageData);
		const sourceIndex = reader.readUint8();
		const isOutput = !!reader.readUint8();

		const extra = ArrayUtilsService.bufferToArray(reader.sliceSized());

		const source = new Source(sourceIndex,
			isOutput,
			ArrayUtilsService.bufferToString(reader.sliceSized()),
			extra);

		this._sourceRequests.get(source.index).next(source);
	}

	private processSourceData(messageData: ArrayBuffer): void {
		const reader = new BufferReader(messageData);

		const timestamp = reader.readUint32();
		const sourcesDataLength = reader.readUint8();
		const sourcesData = new Array(sourcesDataLength);
		for (let sourceIndex = 0; sourceIndex < sourcesDataLength; sourceIndex++) {
			const indexFlags = reader.readUint8();
			// tslint:disable-next-line: no-bitwise
			sourcesData[sourceIndex] = new SourceData(indexFlags & SourceFlags.ClearMask,
				// tslint:disable-next-line: no-bitwise
				indexFlags & SourceFlags.Active,
				reader.slice(SourceData.dataLength));
		}

		this._sourcesSyncSubject.next(new SourcesSync(timestamp, sourcesData));
	}

	public fetch(): void {
		this._fetchingSubject.next(true);
		this.sendSourcesCount();
	}

	public stop(): void {
		this._fetchingSubject.next(false);

		this._boardStateService.unsubscribeSubject(this.subject);
	}

	public requestSourceData(index: number): void {
		this.sendSourceData(index);
	}

	public putExtra(index: number, extra: ReadonlyArray<number>): void {
		this.sendPutExtra(index, extra);
	}

	public get fetching$(): Observable<boolean> {
		return this._fetchingSubject;
	}

	public get canFetch$(): Observable<boolean> {
		return this._canFetchSubject;
	}

	public get sources$(): Observable<ReadonlyArray<Observable<Source>>> {
		return this._sourcesSubjects;
	}

	public get sourcesSync$(): Observable<SourcesSync> {
		return this._sourcesSyncSubject;
	}

	public get syncInterval(): number {
		return this._syncInterval;
	}
}

const autoSubscribe = true;

enum MonitoringCommands {
	SourcesCount = 1,
	Source = 2,
	SourceData = 3,
	PutExtra = 4
}
