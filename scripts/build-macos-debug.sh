mkdir -p out/macos
cp lib/angle/targets/macos/*.dylib out/macos
cc -std=c89 \
  -Wall -Werror -Wpedantic \
  -framework AudioToolbox \
  -framework AudioToolbox \
  -framework Carbon \
  -framework Cocoa \
  -framework CoreAudio \
  -framework CoreFoundation \
  -framework CoreVideo \
  -framework ForceFeedback \
  -framework GameController \
  -framework IOKit \
  -framework CoreHaptics \
  -framework Metal \
  -DMTOTS_ENABLE_SDL=1 \
  -Isrc -Ilib/sdl/include \
  src/*.c lib/sdl/targets/macos/libSDL2.a \
  -fsanitize=address \
  -O0 -g \
  -flto \
  -o out/macos/mtots
