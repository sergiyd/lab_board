import { TestBed } from "@angular/core/testing";

import { DeviceDecoratorService } from "./device-decorator.service";

describe("DeviceDecoratorService", () => {
  beforeEach(() => TestBed.configureTestingModule({}));

  it("should be created", () => {
    const service: DeviceDecoratorService = TestBed.get(DeviceDecoratorService);
    expect(service).toBeTruthy();
  });
});
