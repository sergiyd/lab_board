export interface ChartDataset {
	readonly label: string;
	readonly data: readonly number[];
	readonly fill: boolean;
	readonly borderColor: string;
	readonly lineTension: number;
}
