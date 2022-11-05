import os, shutil

basenames = [
  'mtots_assumptions',
  'mtots_class_ba',
  'mtots_class_class',
  'mtots_class_dict',
  'mtots_class_file',
  'mtots_class_list',
  'mtots_class_str',
  'mtots_compiler',
  'mtots_debug',
  'mtots_dict',
  'mtots_env',
  'mtots_file',
  'mtots_globals',
  'mtots_import',
  'mtots_memory',
  'mtots_modules',
  'mtots_object',
  'mtots_ops',
  'mtots_table',
  'mtots_vm',
]


for basename in basenames:
  cfilename = f'{basename}.c'
  implfilename = f'{basename}_impl.h'
  guard = f'{basename}_impl_h'
  with open(os.path.join('src', cfilename)) as fin:
    contents = fin.read()

  with open(os.path.join('src', implfilename), 'w') as fout:
    fout.write(f'#ifndef {guard}\n')
    fout.write(f'#define {guard}\n')
    fout.write(contents)
    fout.write(f'#endif/*{guard}*/\n')

  os.remove(os.path.join('src', cfilename))
  print(f'#include "{implfilename}"')
