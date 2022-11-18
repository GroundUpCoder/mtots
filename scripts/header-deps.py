"""
Draw a graph of header dependencies
"""
import os
scriptDir = os.path.dirname(os.path.realpath(__file__))
mtotsDir = os.path.dirname(scriptDir)
srcDir = os.path.join(mtotsDir, 'src')
fileNames = [fn for fn in os.listdir(srcDir) if fn.endswith('.h')]

def getDeps(fileName):
  with open(os.path.join(srcDir, fileName)) as f:
    for line in f:
      line = line.strip()
      if line.startswith('#include "'):
        yield line[len('#include "'):-len('"')]

depsMap = {fileName: frozenset(getDeps(fileName)) for fileName in fileNames}

print(depsMap)

