import {
  Component,
  ElementRef,
  Input,
  OnDestroy,
  OnInit,
  ViewChild,
} from "@angular/core";
import { Chart } from "chart.js";
import { Observable, Subject } from "rxjs";
import { takeUntil } from "rxjs/operators";
import { ChartDataset } from "../../models/chart-dataset";

@Component({
  selector: "app-chart",
  templateUrl: "./chart.component.html",
  styleUrls: ["./chart.component.css"],
})
export class ChartComponent implements OnInit, OnDestroy {
  @ViewChild("canvas", { static: true })
  private readonly _canvas: ElementRef;
  private _chart: Chart;
  private _unsubscribe = new Subject();

  private _datasets$: Observable<readonly ChartDataset[]>;
  @Input()
  public set datasets$(datasets: Observable<readonly ChartDataset[]>) {
    this._datasets$ = datasets;
  }

  private _updateAnimationPeriod = defaultUpdateAnimationPeriod;
  @Input()
  public set updateAnimationPeriod(period: number) {
    this._updateAnimationPeriod = period;
  }

  private _stepMs = defaultStepMs;
  @Input()
  public set stepMs(step: number) {
    this._stepMs = step;
  }

  private _deepMs = defaultDeepMs;
  @Input()
  public set deepMs(deep: number) {
    this._deepMs = deep;
  }

  constructor() {}

  public ngOnInit(): void {
    this._chart = new Chart(this._canvas.nativeElement.getContext("2d"), {
      type: "line",
      data: {
        labels: this.labels,
      },
      options: {
        scales: {
          yAxes: [
            {
              display: true,
              position: "right",
              ticks: {
                beginAtZero: true,
              },
            },
          ],
        },
      },
    });

    this._datasets$.pipe(takeUntil(this._unsubscribe)).subscribe((datasets) => {
      this._chart.data.datasets = datasets;
      this._chart.update(this._updateAnimationPeriod);
    });
  }

  public ngOnDestroy(): void {
    this._unsubscribe.next();
    this._unsubscribe.complete();
  }

  private get labels(): readonly string[] {
    return new Array(this._deepMs / this._stepMs)
      .fill(0)
      .map((v, i, a) =>
        (a.length * this._stepMs - i * this._stepMs).toString()
      );
  }
}

const defaultStepMs = 200;
const defaultDeepMs = 10000;
const defaultUpdateAnimationPeriod = 0;
