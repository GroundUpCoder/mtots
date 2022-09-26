cc -std=c89 \
  -Wall -Werror -Wpedantic \
  -Isrc \
  src/*.c \
  -fsanitize=address \
  -O0 -g \
  -flto \
  -o mtots
