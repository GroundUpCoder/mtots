.PHONY: clean test test-macos

$(info ${OS})

# For windows, assume msys2-mingw-clang
ifeq ($(OS),Windows_NT)
	CC = clang
	CCFLAGS = -std=c89 \
		-Wall -Werror -Wpedantic \
		-Isrc \
		-fsanitize-undefined-trap-on-error \
		-O0 -g \
		-o out/desktop/mtots
else
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	CC = clang
	CCFLAGS = -std=c89 \
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
		-DMTOTS_ENABLE_OPENGLES3=1 \
		-Isrc \
		-Ilib/sdl/include \
		-Ilib/angle/include \
		src/*.c \
		lib/sdl/targets/macos/libSDL2.a \
		-fsanitize=address \
		-O0 -g \
		-flto \
		-o out/desktop/mtots 
else
	CC = cc
endif
endif

out/desktop/mtots: src/*
	mkdir -p out/desktop
	$(CC) $(CCFLAGS) src/*.c

out/macos: src/* scripts/*
	mkdir -p out/macos
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
		-DMTOTS_ENABLE_OPENGLES3=1 \
		-Isrc \
		-Ilib/sdl/include \
		-Ilib/angle/include \
		src/*.c \
		lib/sdl/targets/macos/libSDL2.a \
		-fsanitize=address \
		-O0 -g \
		-flto \
		-o out/macos/mtots

out/c89: src/*
	mkdir -p out/c89
	clang -std=c89 \
		-Wall -Werror -Wpedantic \
		-Isrc \
		src/*.c \
		-fsanitize-undefined-trap-on-error \
		-O0 -g \
		-o out/c89/mtots

test: out/c89 scripts/run-tests.py
	python3 scripts/run-tests.py

test-macos: out/macos scripts/run-tests.py
	python3 scripts/run-tests.py macos

test-desktop: out/desktop/mtots scripts/run-tests.py
	python scripts/run-tests.py desktop

out/web: src/* misc/samples/* misc/samples/webgl2/* misc/apps/* root/*
	mkdir -p out/web
	emcc -g src/*.c -Isrc -o out/web/index.html \
		-sUSE_SDL=2 \
    -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 \
		-sASYNCIFY \
		-O3 \
		-DMTOTS_ENABLE_SDL=1 \
		-DMTOTS_ENABLE_OPENGLES3=1 \
		--preload-file misc/samples@/home/web_user/samples \
		--preload-file misc/apps@/home/web_user/apps \
		--preload-file root@/home/web_user/git/mtots/root

clean:
	rm -rf out
