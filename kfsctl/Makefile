
# wget https://raw.githubusercontent.com/luzhixing12345/libc/main/Makefile

CC = gcc
TARGET = kfsctl

CFLAGS = -Wall -Wunused -Werror -Wformat-security -Wshadow -Wpedantic -Wstrict-aliasing -Wuninitialized -Wnull-dereference -Wformat=2

ifneq ($(THIRD_LIB),)
CFLAGS += -I$(THIRD_LIB)/include
LDFLAGS += -L$(THIRD_LIB)/lib
endif

rwildcard = $(foreach d, $(wildcard $1*), $(call rwildcard,$d/,$2) \
						$(filter $2, $d))

SRC = $(call rwildcard, xbox, %.c) main.c
OBJ = $(SRC:.c=.o)
all: $(TARGET)


$(TARGET): $(OBJ)
	$(CC) $^ $(LDFLAGS) -o $@

$(OBJ): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)