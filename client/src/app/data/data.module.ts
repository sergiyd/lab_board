import { CommonModule } from '@angular/common';
import { NgModule } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { ColorPickerModule } from 'ngx-color-picker';
import { LiveComponent } from './components/live/live.component';
import { DataRoutingModule } from './data-routing.module';
import { FormatSourceValuePipe } from './pipes/format-source-value.pipe';
import { FilesComponent } from './components/files/files.component';
import { PlayFileComponent } from './components/play-file/play-file.component';
import { ChartComponent } from './shared/components/chart/chart.component';
import { SourceListComponent } from './shared/components/source-list/source-list.component';
import { HttpClientModule } from '@angular/common/http';

@NgModule({
	declarations: [
		FormatSourceValuePipe,
		LiveComponent,
		FilesComponent,
		PlayFileComponent,
		ChartComponent,
		SourceListComponent
	],
	imports: [
		CommonModule,
		FormsModule,
		DataRoutingModule,
		ColorPickerModule,
		HttpClientModule
	]
})
export class DataModule { }
