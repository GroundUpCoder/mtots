import { MLocation } from "./location";


export abstract class MAst {
  location: MLocation;
  constructor(location: MLocation) {
    this.location = location;
  }
}

export class Nop extends MAst {
  constructor(location: MLocation) {
    super(location);
  }
}
