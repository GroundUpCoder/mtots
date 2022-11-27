import { ParseContext } from "./lang/context";
import { DefaultSourceFinder } from "./sourcefinder";

export const MContext = new ParseContext(DefaultSourceFinder);
