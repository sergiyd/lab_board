export interface ChartDataset {
	readonly label: string;
	readonly data: ReadonlyArray<number>;
	readonly fill: boolean;
	readonly borderColor: string;
	readonly lineTension: number;
}
