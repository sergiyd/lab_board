export enum SourceFlags {
  Output = 2,
  Muted = 8,
	Active = 64,
	// tslint:disable-next-line: no-bitwise
	ClearMask = 0xFF ^ Active
}
