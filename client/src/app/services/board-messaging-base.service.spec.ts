import { TestBed } from "@angular/core/testing";

import { BoardMessagingBaseService } from "./board-messaging-base.service";

describe("BoardMessagingBaseService", () => {
  beforeEach(() => TestBed.configureTestingModule({}));

  it("should be created", () => {
    const service: BoardMessagingBaseService = TestBed.get(
      BoardMessagingBaseService
    );
    expect(service).toBeTruthy();
  });
});
