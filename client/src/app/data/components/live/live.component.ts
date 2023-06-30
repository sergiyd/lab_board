import { Component, OnDestroy, OnInit } from "@angular/core";
import {
  BehaviorSubject,
  combineLatest,
  interval,
  Observable,
  Subject,
  Subscription,
} from "rxjs";
import { map, mergeMap, takeUntil, tap } from "rxjs/operators";
import { SourceData } from "../../models/source-data";
import { SourceFlags } from "../../models/source-flags.enum";
import { DataConvertorService } from "../../services/data-convertor.service";
import { LiveDataService } from "../../services/live-data.service";
import { ChartDataset } from "../../shared/models/chart-dataset";
import { Dataset } from "../../shared/models/dataset";

@Component({
  selector: "app-live",
  templateUrl: "./live.component.html",
  styleUrls: ["./live.component.css"],
})
export class LiveComponent implements OnInit, OnDestroy {
  private readonly _viewDatasets = new Map<number, ViewDataset>();
  private readonly _chartDatasetsSubject = new Subject<
    readonly ChartDataset[]
  >();
  private _datasets$: Observable<readonly Dataset[]>;
  private _sourcesDataSubject = new Map<number, Subject<SourceData>>();
  private _fetching = false;
  private readonly _unsubscribe = new Subject();
  private _lastTimestamp: number;
  private _updateChartSubscription: Subscription;
  private _shiftSubscription: Subscription;
  private _instantSyncIntervalSubject = new BehaviorSubject<number | undefined>(
    undefined
  );

  constructor(private readonly _liveDataService: LiveDataService) {}

  ngOnInit() {
    this._lastTimestamp = 0;
    this._liveDataService.fetching$.subscribe(
      (value) => (this._fetching = value)
    );

    this._datasets$ = this._liveDataService.sources$
      .pipe(takeUntil(this._unsubscribe))
      .pipe(
        mergeMap((sources) => {
          this._viewDatasets.clear();

          return combineLatest(
            sources.map((source) =>
              source.pipe(
                map((s) => new Dataset(s, this.initSourceDataSubject(s.index))),
                tap((dataset) => {
                  if (this._viewDatasets.has(dataset.index)) {
                    this._viewDatasets.get(dataset.index).setDataset(dataset);
                  } else {
                    this._viewDatasets.set(
                      dataset.index,
                      ViewDataset.create(dataset)
                    );
                    this._liveDataService.requestSourceData(dataset.index);
                  }
                  this._chartDatasetsSubject.next(this.chartDatasets);
                })
              )
            )
          );
        })
      );

    this._liveDataService.sourcesSync$
      .pipe(takeUntil(this._unsubscribe))
      .subscribe((sync) => {
        const timestamp = sync.timestamp;

        sync.data
          .filter((sourceData) =>
            this._sourcesDataSubject.has(sourceData.index)
          )
          .forEach((sourceData) => {
            this._sourcesDataSubject.get(sourceData.index).next(sourceData);
          });

        // Zero timestamp if sync was requested explicitly
        if (timestamp) {
          this._instantSyncIntervalSubject.next(
            timestamp - this._lastTimestamp
          );
          this._lastTimestamp = timestamp;
        }
      });
  }

  private initSourceDataSubject(index: number): Observable<SourceData> {
    const result = new Subject<SourceData>();
    this._sourcesDataSubject.set(index, result);

    return result.asObservable();
  }

  ngOnDestroy() {
    if (this._fetching) {
      this.stop();
    }
    this._unsubscribe.next();
    this._unsubscribe.complete();

    Array.from(this._sourcesDataSubject.values()).forEach((v) => v.complete());
  }

  public fetchButtonClick(): void {
    if (this._fetching) {
      this.stop();
    } else {
      this.fetch();
    }
  }

  private get chartDatasets(): readonly ChartDataset[] {
    return Array.from(this._viewDatasets.values()).map(
      (dataset) => dataset.chartDataset
    );
  }

  private fetch(): void {
    this._liveDataService.fetch();
    this._fetching = true;

    this._updateChartSubscription = interval(chartUpdatePeriodMs).subscribe(
      () => this._chartDatasetsSubject.next(this.chartDatasets)
    );

    this._shiftSubscription = interval(this.syncInterval).subscribe(() => {
      const instantSyncInterval = this._instantSyncIntervalSubject.value;
      let skip = 0;
      if (instantSyncInterval > this.syncInterval + syncTimeoutDeltaMs) {
        skip = Math.floor(instantSyncInterval / this.syncInterval);
      }
      this._viewDatasets.forEach((ds) => ds.shift(skip));
    });
  }

  private stop(): void {
    this._liveDataService.stop();
    this._fetching = false;
    this._shiftSubscription.unsubscribe();
    if (!this._updateChartSubscription.closed) {
      this._updateChartSubscription.unsubscribe();
      this._instantSyncIntervalSubject.next(undefined);
    }
  }

  public get canFetch$(): Observable<boolean> {
    return this._liveDataService.canFetch$;
  }

  public get fetching(): boolean {
    return this._fetching;
  }

  public get fetchButtonText(): string {
    return this._fetching ? "Stop" : "Fetch";
  }

  public get instantSyncInterval$(): Observable<number | undefined> {
    return this._instantSyncIntervalSubject;
  }

  public get datasets$(): Observable<readonly Dataset[]> {
    return this._datasets$;
  }

  public get chartDatasets$(): Observable<readonly ChartDataset[]> {
    return this._chartDatasetsSubject;
  }

  public changeDatasetSettings(dataset: Dataset): void {
    this._liveDataService.putExtra(dataset.index, dataset.extra);
  }

  public muteDataset(dataset: Dataset): void {
    if (dataset.unmuted) {
      this._liveDataService.muteSource(dataset.index);
    } else {
      this._liveDataService.unmuteSource(dataset.index);
    }
  }

  public get syncInterval(): number {
    return this._liveDataService.syncInterval;
  }

  public get chartStepMs(): number {
    return chartStepMs;
  }

  public get chartDeepMs(): number {
    return chartDeepMs;
  }
}

const chartStepMs = 200;
const chartDeepMs = 10000;
const chartUpdatePeriodMs = 1000;
const syncTimeoutDeltaMs = 50;

class ViewDataset {
  private _flags: number;
  private _dataset: Dataset;
  private _data: Array<DataView>;
  private _instantData: ArrayBuffer;
  private _sourceDataSubscription: Subscription;

  constructor(dataset: Dataset, length: number) {
    this.setDataset(dataset);
    this._data = new Array<DataView>(length).fill(undefined);
  }

  public static create(dataset: Dataset): ViewDataset {
    return new ViewDataset(dataset, chartDeepMs / chartStepMs);
  }

  public setDataset(dataset: Dataset): void {
    if (this._sourceDataSubscription) {
      this._sourceDataSubscription.unsubscribe();
    }

    this._dataset = dataset;

    this._sourceDataSubscription = this._dataset.sourceData$.subscribe(
      (sourceData) => {
        this._flags = sourceData.flags;
        this._instantData = sourceData.data;
      }
    );
  }

  public shift(skip: number): void {
    // tslint:disable-next-line:no-bitwise
    if (!(this._flags & SourceFlags.Active) || !this._dataset.unmuted) {
      this._instantData = undefined;
    }

    for (let index = 0; index < this._data.length - 1; index++) {
      // Skip with 0 right here
      if (index > this._data.length - skip - 1) {
        this._data[index] = undefined;
      } else {
        this._data[index] = this._data[index + 1];
      }
    }
    this._data[this._data.length - 1] =
      this._instantData && new DataView(this._instantData);
  }

  private get convertedData(): readonly number[] {
    return this._data.map((dataView) =>
      DataConvertorService.convertNumber(
        dataView,
        this._dataset.convertionType,
        this._dataset.formatterCode
      )
    );
  }

  public get chartDataset(): ChartDataset {
    return {
      label: this._dataset.name,
      data: this.convertedData,
      fill: false,
      borderColor: `rgb(${this._dataset.colorRed},${this._dataset.colorGreen},${this._dataset.colorBlue})`,
      lineTension: 0.1,
    };
  }

  public get initialized(): boolean {
    return !!this._dataset;
  }
}
