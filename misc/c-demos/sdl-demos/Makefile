
sdl-demo: main.c
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
		-I../../lib/sdl/include \
		main.c \
		../../lib/sdl/targets/macos/libSDL2.a \
		-fsanitize=address \
		-O0 -g \
		-flto \
		-o sdl-demo

sdl-audio-demo-01: audio-demo-01.c
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
		-I../../lib/sdl/include \
		audio-demo-01.c \
		../../lib/sdl/targets/macos/libSDL2.a \
		-fsanitize=address \
		-O0 -g \
		-flto \
		-o sdl-audio-demo-01

sdl-audio-demo-02: audio-demo-02.c
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
		-I../../lib/sdl/include \
		audio-demo-02.c \
		../../lib/sdl/targets/macos/libSDL2.a \
		-fsanitize=address \
		-O0 -g \
		-flto \
		-o sdl-audio-demo-02
