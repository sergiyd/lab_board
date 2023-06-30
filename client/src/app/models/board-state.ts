import { BoardMode } from "./board-mode.enum";

export class BoardState {
  constructor(
    public readonly mode: BoardMode,
    public readonly syncInterval: number,
    public readonly modeTimestamp: number
  ) {}

  private static _unknown = new BoardState(BoardMode.Unknown, 0, 0);
  public static get unknown(): BoardState {
    return BoardState._unknown;
  }
}
