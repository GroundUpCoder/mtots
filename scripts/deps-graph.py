"""
Draw a graph of header dependencies
"""
import os
import argparse

IGNORE_SET = {
  'mtots_common.h',
  'mtots_debug.h',
  'mtots_debug.c',
}

aparser = argparse.ArgumentParser()
aparser.add_argument('--cfiles', type=int, default=0)
aparser.add_argument('--outfile', '-o', type=str, default=None)
args = aparser.parse_args()

includeCFiles = bool(args.cfiles)
outfilePath = args.outfile

scriptDir = os.path.dirname(os.path.realpath(__file__))
mtotsDir = os.path.dirname(scriptDir)
srcDir = os.path.join(mtotsDir, 'src')
fileNames = [
  fn for fn in os.listdir(srcDir)
  if fn not in IGNORE_SET and (
    fn.endswith('.h') or includeCFiles and fn.endswith('.c'))
]

if outfilePath:
  outfile = open(outfilePath, 'w')
  def emit(string):
    outfile.write(f'{string}\n')
else:
  emit = print

def getDeps(fileName):
  with open(os.path.join(srcDir, fileName)) as f:
    for line in f:
      line = line.strip()
      if line.startswith('#include "'):
        depFileName = line[len('#include "'):-len('"')]
        if depFileName not in IGNORE_SET:
          yield depFileName

def formatName(name):
  if name.endswith('.h') and name.startswith('mtots_'):
    return name[len('mtots_'):-len('.h')]
  return name.replace('.', '_')

depsMap = {
  formatName(fileName): sorted(formatName(d) for d in getDeps(fileName)) for fileName in fileNames
}

# Remove the connection where source files depend on their corresponding header
for target in depsMap:
  if not (target.startswith('mtots_') and target.endswith('_c')):
    continue
  basename = target[len('mtots_'):-len('_c')]
  if basename in depsMap[target]:
    depsMap[target].remove(basename)

emit("digraph headerDeps {")

for target in depsMap:
  if target.endswith('_c'):
    emit(f'  {target} [shape=box]')

for target in depsMap:
  for dep in depsMap[target]:
    emit(f'  {target} -> {dep}')
emit("}")

if outfilePath:
  outfile.close()
