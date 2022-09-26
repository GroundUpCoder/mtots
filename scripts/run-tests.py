"""
Test runner requires Python3
"""
import os, sys, subprocess

repoDir = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
testDir = os.path.join(repoDir, 'test')
mtotsPath = os.path.join(repoDir, 'mtots')

dirnames = sorted(os.listdir(testDir))

testSetCount = len(dirnames)
testCount = passCount = 0

print(f'Found {testSetCount} test set(s)')

for testSet in dirnames:
  testSetDir = os.path.join(testDir, testSet)
  filenames = sorted(os.listdir(testSetDir))
  scriptFilenames = [fn for fn in filenames if fn.endswith('.mtots')]

  print(f'  {testSet}')

  for sfn in scriptFilenames:
    base = sfn[:-len('.mtots')]

    scriptPath = os.path.join(testSetDir, sfn)

    expectExit = 0
    expectExitFn = os.path.join(testSetDir, f'{base}.exit.txt')
    if os.path.exists(expectExitFn):
      with open(expectExitFn) as f:
        expectExitStr = f.read().strip()
        if expectExitStr.lower() == 'any':
          expectExit = None
        else:
          expectExit = int(expectExitStr)

    expectOut = ''
    expectOutFn = os.path.join(testSetDir, f'{base}.out.txt')
    if os.path.exists(expectOutFn):
      with open(expectOutFn) as f:
        expectOut = f.read()

    expectErr = ''
    expectErrFn = os.path.join(testSetDir, f'{base}.err.txt')
    if os.path.exists(expectErrFn):
      with open(expectErrFn) as f:
        expectErr = f.read()

    sys.stdout.write(f'    testing {base}... ')

    proc = subprocess.run(
      [mtotsPath, scriptPath],
      capture_output=True,
      text=True)

    if expectExit is not None and proc.returncode != expectExit:
      print('FAILED (exit code)')
      print('##### Expected #####')
      print(expectExit)
      print('##### TO EQUAL #####')
      print(proc.returncode)
      print(f'##### STDOUT: #####')
      print(proc.stdout)
      print(f'##### STDERR: #####')
      print(proc.stderr)
    elif expectOut != proc.stdout:
      print('FAILED (stdout)')
      print('##### Expected #####')
      print(proc.stdout)
      print('##### TO EQUAL #####')
      print(expectOut)
    elif expectErr != proc.stderr:
      print('FAILED (stderr)')
      print('##### Expected #####')
      print(proc.stderr)
      print('##### TO EQUAL #####')
      print(expectOut)
    else:
      passCount += 1
      print("OK")
    testCount += 1

if passCount == testCount:
  print("ALL TESTS PASS")
else:
  print("Some tests failed:")
  print(f"  {passCount} / {testCount}")
  print(f"  {testCount - passCount} test(s) failed")

exit(0 if passCount == testCount else 1)
