import type { MLocation } from "./location";
import type * as type from "./type";



export class MSignatureHelper {
  readonly location: MLocation;
  readonly parameterTypes: type.MType[];
  readonly parameterIndex: number;
  constructor(location: MLocation, parameterTypes: type.MType[], parameterIndex: number) {
    this.location = location;
    this.parameterTypes = parameterTypes;
    this.parameterIndex = parameterIndex;
  }
}
