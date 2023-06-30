import { HttpClient, HttpEventType } from '@angular/common/http';
import { Component, OnDestroy, OnInit } from '@angular/core';
import { ActivatedRoute } from '@angular/router';
import { BehaviorSubject, interval, Observable, Subject } from 'rxjs';
import { map, share, takeUntil } from 'rxjs/operators';
import { Source } from '../../models/source';
import { SourceData } from '../../models/source-data';
import { SourcesSync } from '../../models/sources-sync';
import { DataConvertorService } from '../../services/data-convertor.service';
import { ChartDataset } from '../../shared/models/chart-dataset';
import { Dataset } from '../../shared/models/dataset';
import { ConfigResolverService } from 'src/app/services/config-resolver.service';

@Component({
	selector: 'app-play-file',
	templateUrl: './play-file.component.html',
	styleUrls: ['./play-file.component.css']
})
export class PlayFileComponent implements OnInit, OnDestroy {
	private readonly _chartDatasetsSubject = new BehaviorSubject<readonly ChartDataset[]>([]);
	private readonly _datasetsSubject = new BehaviorSubject<readonly Dataset[]>([]);
  private readonly _boardUrl: string;
	private _name: string;
	private _loadedSubject = new Subject<boolean>();
	private _fileData: FileData;
	private _downloadProgressSubject = new Subject<DownloadProgress>();
	private readonly _sourceDataSubject = new Subject<SourcesSync>();
	private readonly _sourceData$ = this._sourceDataSubject.pipe(share());
	private _position: number;
	private _chartDataMatix: ReadonlyMap<number, Array<readonly number[]>>;
	private readonly _playerActionSubject = new Subject<PlayerAction>();
	public readonly playerActionBegin = PlayerAction.Begin;
	public readonly playerActionPlayForward = PlayerAction.PlayForward;
	public readonly playerActionStepForward = PlayerAction.StepForward;
	public readonly playerActionPlayBackward = PlayerAction.PlayBackward;
	public readonly playerActionStepBackward = PlayerAction.StepBackward;
	public readonly playerActionEnd = PlayerAction.End;
	public readonly playerActionStop = PlayerAction.Stop;
	private readonly _unsubscribe = new Subject();
	public readonly playerAction$ = this._playerActionSubject.asObservable();

	constructor(private readonly _route: ActivatedRoute,
		private readonly _httpClient: HttpClient,
    configResolverService: ConfigResolverService) {
      this._boardUrl = configResolverService.resolve().boardUrl;      
    }

	public ngOnInit(): void {
		this._name = this._route.snapshot.paramMap.get('name');

		this._downloadProgressSubject.next(new DownloadProgress(0));
		this
			._httpClient
			.get(`${this._boardUrl}/data-file?name=${this._name}`, {
				observe: 'events',
				responseType: 'json',
				reportProgress: true
			})
			.subscribe(event => {
				switch (event.type) {
					case HttpEventType.DownloadProgress:
						this._downloadProgressSubject.next(new DownloadProgress(event.loaded, event.total));
						return;
					case HttpEventType.Response:
						this.setData(event.body as FileData);
						this._loadedSubject.next(true);
						return;
				}
			});

		this
			._playerActionSubject
			.pipe(takeUntil(this._unsubscribe))
			.subscribe(action => {
				switch (action) {
					case PlayerAction.Begin:
						this.position = 0;
						return;
					case PlayerAction.PlayForward:
					case PlayerAction.PlayBackward:
						interval(this.chartStepMs)
							.pipe(takeUntil(this._unsubscribe), takeUntil(this._playerActionSubject))
							.subscribe(() => this.step(action === PlayerAction.PlayForward));
						return;
					case PlayerAction.StepForward:
					case PlayerAction.StepBackward:
						this.step(action === PlayerAction.StepForward);
						return;
					case PlayerAction.End:
						this.position = this.maxPosition;
						return;
				}
			});
	}

	private step(forward: boolean): void {
		const nextPosition = this.position + this.chartStepMs * (forward ? 1 : -1);

		if (nextPosition < 0 || nextPosition > this.maxPosition) {
			this._playerActionSubject.next();
		} else {
			this.position = nextPosition;
		}
	}

	public ngOnDestroy(): void {
		this._unsubscribe.next();
		this._unsubscribe.complete();
	}

	public get name(): string {
		return this._name;
	}

	public get datasets$(): Observable<readonly Dataset[]> {
		return this._datasetsSubject.asObservable();
	}

	public get chartDatasets$(): Observable<readonly ChartDataset[]> {
		return this._chartDatasetsSubject;
	}

	public get loaded$(): Observable<boolean> {
		return this._loadedSubject;
	}

	public playerAction(action: PlayerAction): void {
		this._playerActionSubject.next(action);
	}

	public get downloadProgress$(): Observable<DownloadProgress> {
		return this._downloadProgressSubject;
	}

	public get chartStepMs(): number {
		return this._fileData?.period;
	}

	public get chartDeepMs(): number {
		return chartDeepMs;
	}

	private setData(fileData: FileData): void {
		this._fileData = fileData;

		this._chartDataMatix = new Map<number, Array<readonly number[]>>(this
			._fileData
			.devices
			.map(device => [device.index, new Array<readonly number[]>(chartDeepMs / this.chartStepMs)]));

    const isOutput = false;
    const muted = false;

		const datasets = this
			._fileData
			.devices
			.map(device => {
        const source = new Source(device.index, isOutput, muted, device.name, device.extra);
				return new Dataset(source,
					this._sourceData$.pipe(map(sourcesSync => sourcesSync.data.find(data => data.index === source.index))));
			});

		this._datasetsSubject.next(datasets);

		this.position = 0;
	}

	public get maxPosition(): number {
		const data = this._fileData.data;
		if (data?.length) {
			const rawMaxPosition = data[data.length - 1].t - this._fileData.started;
			return Math.round(rawMaxPosition / this.chartStepMs) * this.chartStepMs;
		} else {
			return 0;
		}
	}

	public get position(): number {
		return this._position;
	}

	public set position(rawPosition: number) {
		const started = this._fileData.data[0].t;
		this._position = Math.min(rawPosition, this.maxPosition);

		// Clear values
		Array
			.from(this._chartDataMatix.keys())
			.forEach(deviceIndex => this._chartDataMatix.get(deviceIndex).fill(undefined));

		let matrixIndex = 0;
		const matrixMaxIndex = (chartDeepMs / this.chartStepMs) - 1;
		for (const record of this._fileData.data) {
			const recordTime = record.t - started;
			if (recordTime > this._position) {
				// Record is beyond the highest time bound
				break;
			} else if (recordTime < this._position - chartDeepMs) {
				// Record is beyond the lowest time bound
				continue;
			} else if (recordTime <= this._position) {
				matrixIndex = matrixMaxIndex - ((this._position - recordTime) / this.chartStepMs);

				if (Math.abs(recordTime - this._position) < this.chartStepMs) {
					this
						._sourceDataSubject
						.next(new SourcesSync(record.t, record.d.map(d => new SourceData(d.i, d.f, new Uint8Array(d.v).buffer))));
				}
				record
					.d
					.forEach(d => this._chartDataMatix.get(d.i)[Math.floor(Math.max(0, matrixIndex))] = d.v);
			}
		}

		const chartDatasets = this
			._datasetsSubject
			.value
      .filter(dataset => dataset.unmuted)
			.map(dataset => {
				return {
					label: dataset.name,
					data: this
						._chartDataMatix
						.get(dataset.index)
						.map(values => {
							const dataView = values && new DataView(new Uint8Array(values).buffer);
							return DataConvertorService.convertNumber(dataView, dataset.convertionType, dataset.formatterCode);
						}),
					fill: false,
					borderColor: `rgb(${dataset.colorRed},${dataset.colorGreen},${dataset.colorBlue})`,
					lineTension: 0.1
				} as ChartDataset;
			});

		this._chartDatasetsSubject.next(chartDatasets);
	}

  public muteDataset(dataset: Dataset): void {
    // Doesn't work with this component in two-sidemodel bind mode
    dataset.unmuted = !dataset.unmuted;
    this.position = this.position;
	}
}

const chartDeepMs = 10000;

interface FileData {
	readonly started: number;
	readonly period: number;
	readonly devices: readonly FileDataDevice[];
	readonly data: readonly FileDataBatch[];
}

interface FileDataDevice {
	readonly index: number;
	readonly flags: number;
	readonly bus: number;
	readonly type: number;
	readonly name: string;
	readonly address: readonly number[];
	readonly extra: readonly number[];
	readonly data: readonly number[];
}

interface FileDataBatch {
	readonly t: number;
	readonly d: readonly FileDataRecord[];
}

interface FileDataRecord {
	readonly i: number;
	readonly f: number;
	readonly v: readonly number[];
}

class DownloadProgress {
	public readonly percents?: number;

	constructor(public readonly loaded: number,
		public readonly total?: number) {
		if (total) {
			this.percents = loaded / (total / 100);
		}
	}
}

enum PlayerAction {
	Begin,
	PlayForward,
	StepForward,
	PlayBackward,
	StepBackward,
	End,
	Stop
}
