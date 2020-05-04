export interface BoardServiceConfig {
	readonly url: string;
	readonly reconnectTimeoutMs: number;
	readonly reconnectAttempts: number;
}
