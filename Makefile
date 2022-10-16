.PHONY: clean test test-macos

out/macos: src/* scripts/*
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
		-DMTOTS_ENABLE_OPENGLES3=1 \
		-Isrc \
		-Ilib/sdl/include \
		-Ilib/angle/include \
		src/*.c \
		lib/sdl/targets/macos/libSDL2.a \
		lib/angle/targets/macos/libEGL.dylib \
		lib/angle/targets/macos/libGLESv2.dylib \
		-fsanitize=address \
		-O0 -g \
		-flto \
		-o out/macos/mtots

out/c89: src/*
	mkdir -p out/c89
	cc -std=c89 \
		-Wall -Werror -Wpedantic \
		-Isrc \
		src/*.c \
		-fsanitize=address \
		-O0 -g \
		-flto \
		-o out/c89/mtots

test: out/c89 scripts/run-tests.py
	python3 scripts/run-tests.py

test-macos: out/macos scripts/run-tests.py
	python3 scripts/run-tests.py macos

out/web: src/* misc/new-samples/* misc/apps/* root/*
	mkdir -p out/web
	emcc -g src/*.c -Isrc -o out/web/index.html \
		-sUSE_SDL=2 \
    -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 \
		-sASYNCIFY \
		-O3 \
		-DMTOTS_ENABLE_SDL=1 \
		-DMTOTS_ENABLE_OPENGLES3=1 \
		--preload-file misc/new-samples@/home/web_user/samples \
		--preload-file misc/apps@/home/web_user/apps \
		--preload-file root@/home/web_user/git/mtots/root

clean:
	rm -rf out
