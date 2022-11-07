
/* Eventually mtots core will be implemented in this translation unit */

#include "mtots_unicode_impl.h"
#include "mtots_str_impl.h"
#include "mtots_scanner_impl.h"
#include "mtots_chunk_impl.h"
#include "mtots_value_impl.h"
#include "mtots_assumptions_impl.h"
#include "mtots_class_ba_impl.h"
#include "mtots_class_class_impl.h"
#include "mtots_class_dict_impl.h"
#include "mtots_class_file_impl.h"
#include "mtots_class_list_impl.h"
#include "mtots_class_str_impl.h"
#include "mtots_compiler_impl.h"
#include "mtots_debug_impl.h"
#include "mtots_dict_impl.h"
#include "mtots_env_impl.h"
#include "mtots_file_impl.h"
#include "mtots_globals_impl.h"
#include "mtots_import_impl.h"
#include "mtots_memory_impl.h"
#include "mtots_modules_impl.h"
#include "mtots_object_impl.h"
#include "mtots_ops_impl.h"
#include "mtots_vm_impl.h"
#include "mtots_ref_impl.h"

const char *getErrorString() {
  return vm.errorString;
}
