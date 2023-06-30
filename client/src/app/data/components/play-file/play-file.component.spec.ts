import { async, ComponentFixture, TestBed } from "@angular/core/testing";

import { PlayFileComponent } from "./play-file.component";

describe("PlayFileComponent", () => {
  let component: PlayFileComponent;
  let fixture: ComponentFixture<PlayFileComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [PlayFileComponent],
    }).compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(PlayFileComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it("should create", () => {
    expect(component).toBeTruthy();
  });
});
