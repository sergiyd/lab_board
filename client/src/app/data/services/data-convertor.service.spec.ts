import { TestBed } from '@angular/core/testing';

import { DataConvertorService } from './data-convertor.service';

describe('DataConvertorService', () => {
  beforeEach(() => TestBed.configureTestingModule({}));

  it('should be created', () => {
    const service: DataConvertorService = TestBed.get(DataConvertorService);
    expect(service).toBeTruthy();
  });
});
