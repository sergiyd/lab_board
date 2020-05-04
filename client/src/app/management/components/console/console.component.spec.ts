import { async, ComponentFixture, TestBed } from '@angular/core/testing';
import { ManagementConsoleComponent } from './console.component';

describe('ManagementConsoleComponent', () => {
	let component: ManagementConsoleComponent;
	let fixture: ComponentFixture<ManagementConsoleComponent>;

	beforeEach(async(() => {
		TestBed.configureTestingModule({
			declarations: [ManagementConsoleComponent]
		})
			.compileComponents();
	}));

	beforeEach(() => {
		fixture = TestBed.createComponent(ManagementConsoleComponent);
		component = fixture.componentInstance;
		fixture.detectChanges();
	});

	it('should create', () => {
		expect(component).toBeTruthy();
	});
});
