import { MPosition } from "./position";


export class MRange {
  start: MPosition;
  end: MPosition;

  constructor(start: MPosition, end: MPosition) {
    this.start = start;
    this.end = end;
  }

  merge(other: MRange) {
    return new MRange(
      (this.start.lt(other.start) ? this.start : other.start).clone(),
      (this.end.lt(other.end) ? other.end : this.end).clone());
  }
}
