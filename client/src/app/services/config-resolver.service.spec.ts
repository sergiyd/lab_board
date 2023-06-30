import { TestBed } from "@angular/core/testing";
import { ConfigResolverService } from "./config-resolver.service";

describe("ConfigResolverService", () => {
  let service: ConfigResolverService;

  beforeEach(() => {
    TestBed.configureTestingModule({});
    service = TestBed.inject(ConfigResolverService);
  });

  it("should be created", () => {
    expect(service).toBeTruthy();
  });
});
