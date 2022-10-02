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
		misc/sdl-demo/main.c \
		lib/sdl/targets/macos/libSDL2.a \
		-fsanitize=address \
		-O0 -g \
		-flto \
		-o sdl-demo

sdl-audio-demo-01: src/* misc/sdl-demo/*
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
		misc/sdl-demo/audio-demo-01.c \
		lib/sdl/targets/macos/libSDL2.a \
		-fsanitize=address \
		-O0 -g \
		-flto \
		-o sdl-audio-demo-01

sdl-audio-demo-02: src/* misc/sdl-demo/*
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
		misc/sdl-demo/audio-demo-02.c \
		lib/sdl/targets/macos/libSDL2.a \
		-fsanitize=address \
		-O0 -g \
		-flto \
		-o sdl-audio-demo-02

clean:
	rm -rf mtots sdl-demo
