cc -std=c89 \
  -Wall -Werror -Wpedantic \
  -Isrc \
  src/*.c \
  -O3 \
  -flto \
  -o mtots
