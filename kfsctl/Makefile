
# wget https://raw.githubusercontent.com/luzhixing12345/libc/main/Makefile

CC = gcc
TARGET = kfsctl

THIRD_LIB = fuse3
CFLAGS += $(shell pkg-config --cflags $(THIRD_LIB))
LDFLAGS += $(shell pkg-config --libs $(THIRD_LIB))

CSTD = -std=gnu2x
CFLAGS = $(CSTD)
CFLAGS += -Wall -Wunused -Werror -Wformat-security -Wshadow -Wpedantic -Wstrict-aliasing -Wuninitialized -Wnull-dereference -Wformat=2
CFLAGS += -Wno-error=pedantic -Wno-pedantic -Wno-format-nonliteral -g

CFLAGS += -I../src

rwildcard = $(foreach d, $(wildcard $1*), $(call rwildcard,$d/,$2) \
						$(filter $2, $d))

SRC := $(shell find . -name '*.c')
OBJ = $(SRC:.c=.o)
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $^ $(LDFLAGS) -o $@

$(OBJ): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)