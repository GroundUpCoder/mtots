# New Object Type Checklist

Checklist of code that needs to be updated whenever
a new object type is created.

* In `object.h`/`object.c`
  * New entry in `ObjType` enum
  * `IS_*` macro
  * `AS_*` macro
  * `new*` function
  * `printObject`
  * `getObjectTypeName`
* In `value.h`/`value.c`
  * `getKindName`
* In `memory.c`
  * `freeObject` how to free object of given type
  * `blackenObject` trace through for GC
* In `globals.c`
  * `implRepr`
