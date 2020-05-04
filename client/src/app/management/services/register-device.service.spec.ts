import { TestBed } from '@angular/core/testing';

import { RegisterDeviceService } from './register-device.service';

describe('RegisterDeviceService', () => {
  beforeEach(() => TestBed.configureTestingModule({}));

  it('should be created', () => {
    const service: RegisterDeviceService = TestBed.get(RegisterDeviceService);
    expect(service).toBeTruthy();
  });
});
