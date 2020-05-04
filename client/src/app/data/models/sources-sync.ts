import { SourceData } from './source-data';

export class SourcesSync {
	constructor(public readonly timestamp: number,
		public readonly data: ReadonlyArray<SourceData>) {}
}
