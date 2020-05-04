import { Pipe, PipeTransform } from '@angular/core';
import { DataConvertorService } from '../services/data-convertor.service';

@Pipe({
	name: 'formatSourceValue'
})
export class FormatSourceValuePipe implements PipeTransform {

	transform(value: any, ...args: any[]): any {
		return value ?
			DataConvertorService.format(new DataView(value), args[0], args[1]) :
			'-';
	}
}
