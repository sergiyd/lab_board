import { Component, Input, OnDestroy, OnInit, EventEmitter, Output } from '@angular/core';
import { Observable, Subject } from 'rxjs';
import { LiveDataService } from 'src/app/data/services/live-data.service';
import { Dataset } from '../../models/dataset';
import { DataConvertorService } from 'src/app/data/services/data-convertor.service';
import { FormatterCodes } from 'src/app/data/models/formatter-codes.enum';
import { ConvertionTypes } from 'src/app/data/models/convertion-types.enum';

@Component({
	selector: 'app-source-list',
	templateUrl: './source-list.component.html',
	styleUrls: ['./source-list.component.css']
})
export class SourceListComponent implements OnInit, OnDestroy {
	public readonly formatters: ReadonlyArray<FormatterItem> =
		Array.from(DataConvertorService.getFormatters()).map(item => new FormatterItem(item[0], item[1]));
	public readonly convertors: ReadonlyArray<ConvertorItem> =
		Array.from(DataConvertorService.getConvertors()).map(item => new ConvertorItem(item[0], item[1]));

	private readonly _unsubscribe = new Subject();
	private _datasets$: Observable<ReadonlyArray<Dataset>>;

	public get datasets$(): Observable<ReadonlyArray<Dataset>> {
		return this._datasets$;
	}
	@Input()
	public set datasets$(datasets: Observable<ReadonlyArray<Dataset>>) {
		this._datasets$ = datasets;
	}

	@Output()
	public readonly changeDatasetSettings = new EventEmitter<Dataset>();

	constructor(private readonly _liveDataService: LiveDataService) { }

	ngOnInit(): void {

	}

	ngOnDestroy(): void {
		this._unsubscribe.next();
		this._unsubscribe.complete();
	}

	public changeSettings(dataset: Dataset): void {
		this.changeDatasetSettings.emit(dataset);
	}
}

class FormatterItem {
	constructor(public readonly code: FormatterCodes,
		public readonly name: string) { }
}

class ConvertorItem {
	constructor(public readonly type: ConvertionTypes,
		public readonly name: string) { }
}
