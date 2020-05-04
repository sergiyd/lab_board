import { BoardMessage } from './board-message';

describe('BoardMessage', () => {
  it('should create an instance', () => {
    expect(new BoardMessage(Number.MIN_VALUE)).toBeTruthy();
  });
});
