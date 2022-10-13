mkdir -p out/c89
cc -std=c89 \
  -Wall -Werror -Wpedantic \
  -Isrc \
  src/*.c \
  -fsanitize=address \
  -O0 -g \
  -flto \
  -o out/c89/mtots
