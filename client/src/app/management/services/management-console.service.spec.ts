import { TestBed } from '@angular/core/testing';

import { ManagementConsoleService } from './management-console.service';

describe('ManagementConsoleService', () => {
	beforeEach(() => TestBed.configureTestingModule({}));

	it('should be created', () => {
		const service: ManagementConsoleService = TestBed.get(ManagementConsoleService);
		expect(service).toBeTruthy();
	});
});
