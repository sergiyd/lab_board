export enum SourceFlags {
	Active = 64,
	// tslint:disable-next-line: no-bitwise
	ClearMask = 0xFF ^ Active
}
