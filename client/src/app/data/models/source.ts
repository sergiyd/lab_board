export class Source {
	constructor(public readonly index: number,
		public readonly output: boolean,
    public readonly muted: boolean,
		public readonly name: string,
		public readonly extra: ReadonlyArray<number>) {}
}
