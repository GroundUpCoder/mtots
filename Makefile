.PHONY: clean

mtots: src/* scripts/*
	sh scripts/build-macos-debug.sh

sdl-demo: src/* misc/sdl-demo/*
	cc -std=c89 \
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
		-Wall -Werror -Wpedantic \
		-Isrc \
		-Ilib/sdl/include \
		misc/sdl-demo/*.c \
		lib/sdl/targets/macos/libSDL2.a \
		-fsanitize=address \
		-O0 -g \
		-flto \
		-o sdl-demo

clean:
	rm -rf mtots sdl-demo
