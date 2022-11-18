import { MPosition } from "./position";


export class MRange {
  start: MPosition;
  end: MPosition;

  constructor(start: MPosition, end: MPosition) {
    this.start = start;
    this.end = end;
  }
}
