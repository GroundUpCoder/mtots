.PHONY: all clean web run

all: out

out-macos: *.c
	mkdir -p out-macos
	cp ../../../lib/angle/targets/macos/*.dylib out-macos
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
		-I../../../lib/sdl/include \
		-I../../../lib/angle/include \
		*.c \
		../../../lib/sdl/targets/macos/libSDL2.a \
		-fsanitize=address \
		-O0 -g \
		-o out-macos/a.out

web: out-web

out-web: *.c
	mkdir -p out-web
	emcc -g *.c -o out-web/index.html \
		-sUSE_SDL=2 \
    -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 \
		-sASYNCIFY \
		-O3

clean:
	rm -rf out-macos out-web
