export interface BoardServiceConfig {
	readonly wsUrl: string;
	readonly boardUrl: string;
	readonly reconnectTimeoutMs: number;
	readonly reconnectAttempts: number;
}
