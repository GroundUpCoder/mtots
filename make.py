"""In liu of a Makefile, I think a Python file is for the most part preferable.
"""
import argparse
import os
import shutil
import sys
import subprocess

print(sys.argv[0])
join = os.path.join
mtotsDir = os.path.dirname(os.path.realpath(__file__))

TARGETS = (
    # Phony targets
    'test',
    'clean',

    # Buildable targets
    'desktop',
    'web',
)

aparser = argparse.ArgumentParser()
aparser.add_argument('target', default='test', nargs='?', choices=TARGETS)

args = aparser.parse_args()

target: str = args.target


def getSources():
    srcs = []
    srcDir = join(mtotsDir, 'src')
    for filename in os.listdir(srcDir):
        if filename.endswith('.c'):
            srcs.append(join(srcDir, filename))
    return srcs


def run(args):
    try:
        subprocess.run(args, check=True)
    except subprocess.CalledProcessError as e:
        sys.stderr.write(f'Failed process:\n')
        sys.stderr.write(f'  {args[0]}\n')
        for arg in args[1:]:
            sys.stderr.write(f'    {arg}\n')
        sys.stderr.write(f'Return code: {e.returncode}\n')
        sys.exit(1)


def buildDesktop():
    os.makedirs(join(mtotsDir, 'out', 'desktop'), exist_ok=True)
    if sys.platform.startswith('win32'):
        # windows
        run([
            join(mtotsDir, 'scripts', 'msvc.bat'),
            '/Za',
            '/I' + join(mtotsDir, 'src'),
            '/Fd' + join(mtotsDir, 'out', 'desktop') + '\\',
            '/Fo' + join(mtotsDir, 'out', 'desktop') + '\\',
            '/Fe' + join(mtotsDir, 'out', 'desktop', 'mtots'),
            '/Zi',
            '/fsanitize=address',
        ] + getSources())
    elif sys.platform.startswith('darwin'):
        # MacOS
        run([
            'clang',
            "-std=c89",
            "-Wall", "-Werror", "-Wpedantic",
            "-framework", "AudioToolbox",
            "-framework", "AudioToolbox",
            "-framework", "Carbon",
            "-framework", "Cocoa",
            "-framework", "CoreAudio",
            "-framework", "CoreFoundation",
            "-framework", "CoreVideo",
            "-framework", "ForceFeedback",
            "-framework", "GameController",
            "-framework", "IOKit",
            "-framework", "CoreHaptics",
            "-framework", "Metal",
            "-DMTOTS_ENABLE_SDL=1",
            "-DMTOTS_ENABLE_OPENGLES3=1",
            "-Isrc",
            "-Ilib/sdl/include",
            "-Ilib/angle/include",
            "lib/sdl/targets/macos/libSDL2.a",
            "-fsanitize=address",
            "-O0", "-g",
            "-flto",
            "-o", join(mtotsDir, "out", "desktop", "mtots"),
        ] + getSources())
    elif sys.platform.startswith('linux'):
        # Linux
        raise "TODO"
    else:
        # Assume some sort of unix
        raise "TODO"


def runTests():
    exe = (
        'python' if sys.platform.startswith('win32') else
        'python3'
    )
    run([exe, join(mtotsDir, 'scripts', 'run-tests.py'), 'desktop'])


if target == 'clean':
    shutil.rmtree(join(mtotsDir, 'out'), ignore_errors=True)
elif target == 'desktop':
    buildDesktop()
elif target == 'test':
    buildDesktop()
    runTests()
else:
    raise Exception(f"Unrecognized target {target}")
