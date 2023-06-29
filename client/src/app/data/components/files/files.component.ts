import { Component } from '@angular/core';
import { Observable } from 'rxjs';
import { DataFile } from '../../models/data-file';
import { FileDataService } from '../../services/file-data.service';
import { ConfigResolverService } from 'src/app/services/config-resolver.service';

@Component({
  selector: 'app-files',
  templateUrl: './files.component.html',
  styleUrls: ['./files.component.css']
})
export class FilesComponent {
  public readonly files$: Observable<ReadonlyArray<DataFile>>;
  public readonly boardUrl: string;

  constructor(private readonly _fileDataService: FileDataService,
    configResolverService: ConfigResolverService) {
    this.files$ = this._fileDataService.files$;
    this.boardUrl = configResolverService.resolve().boardUrl;
  }

  public get idleMode$(): Observable<boolean> {
    return this._fileDataService.idleMode$;
  }

  public removeFile(name: string) {
    this._fileDataService.removeFile(name);
  }
}
