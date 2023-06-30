export interface Formatter {
  prefix?: string;
  suffix?: string;
  formatNumber(value: number): number;
  formatString(value: number): string;
  format(value: number): string;
}
