import { Component } from '@angular/core';
import { Observable } from 'rxjs';
import { DataFile } from '../../models/data-file';
import { FileDataService } from '../../services/file-data.service';

@Component({
	selector: 'app-files',
	templateUrl: './files.component.html',
	styleUrls: ['./files.component.css']
})
export class FilesComponent {
	public readonly files$: Observable<ReadonlyArray<DataFile>>;

	constructor(private readonly _fileDataService: FileDataService) {
		this.files$ = this._fileDataService.files$;
	}

	public get idleMode$(): Observable<boolean> {
		return this._fileDataService.idleMode$;
	}

	public removeFile(name: string) {
		this._fileDataService.removeFile(name);
	}
}
