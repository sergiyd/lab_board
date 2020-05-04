import { Injectable } from '@angular/core';
import { BoardMessage } from '../models/board-message';
import { BoardService } from './board.service';

@Injectable({
	providedIn: 'root'
})
export abstract class BoardMessagingBaseService {

	constructor(protected readonly boardService: BoardService) { }

	protected abstract get subject(): number;

	protected sendCommand(command: number, data?: ArrayBuffer): void {
		this.boardService
			.send(this.subject, new BoardMessage(command, data));
	}

	protected subscribeOnQueue(): void {
		this.boardService
			.queue$(this.subject)
			.subscribe(this.processMessage.bind(this));
	}

	protected abstract processMessage(boardMessage: BoardMessage): void;
}
