import type { MLocation } from "./location";
import type * as type from "./type";



export class MSignatureHelper {
  readonly location: MLocation;
  readonly functionName: string | null;
  readonly functionDocumentation: string | null;
  readonly parameterTypes: type.MType[];
  readonly parameterNames: string[] | null;
  readonly parameterIndex: number;
  readonly returnType: type.MType | null;
  constructor(
      location: MLocation,
      functionName: string | null,
      functionDocumentation: string | null,
      parameterTypes: type.MType[],
      parameterNames: string[] | null,
      parameterIndex: number,
      returnType: type.MType | null) {
    this.location = location;
    this.functionName = functionName;
    this.functionDocumentation = functionDocumentation;
    this.parameterTypes = parameterTypes;
    this.parameterNames = parameterNames;
    this.parameterIndex = parameterIndex;
    this.returnType = returnType;
  }
}
