
# wget https://raw.githubusercontent.com/luzhixing12345/libc/main/Makefile

CC = gcc
TARGET = kfs
SRC_PATH = src
SRC_EXT = c
THIRD_LIB = fuse3 zlib

CSTD = -std=gnu2x
CFLAGS = $(CSTD)
CFLAGS += -Wall -Wunused -Werror -Wformat-security -Wshadow -Wpedantic -Wstrict-aliasing -Wuninitialized -Wnull-dereference -Wformat=2

# 使用 GNU C 扩展语法, __FILE__ 和 __LINE__ 变量
CFLAGS += -Wno-error=pedantic -Wno-pedantic -Wno-format-nonliteral -Wno-error=unused-result

CFLAGS += $(shell pkg-config --cflags $(THIRD_LIB))
LDFLAGS += $(shell pkg-config --libs $(THIRD_LIB))
CFLAGS += -I$(INCLUDE_PATH)

TEST_PATH = test
TMP_PATH = tmp
# 项目名字(库)
RELEASE = $(TARGET)
LIB = lib$(TARGET).a
# ------------------------- #

rwildcard = $(foreach d, $(wildcard $1*), $(call rwildcard,$d/,$2) \
						$(filter $2, $d))

SRC = $(call rwildcard, $(SRC_PATH), %.$(SRC_EXT))
OBJ = $(SRC:.$(SRC_EXT)=.o)

MKFS = mkfs
# MKFS = mkfs.ext4

MKFS_SRC_PATH = mkfs
MKFS_SRC = $(call rwildcard, $(MKFS_SRC_PATH), %.$(SRC_EXT))
MKFS_OBJ = $(MKFS_SRC:.$(SRC_EXT)=.o)

DUMPFS = dumpe2fs

OPTIM_LEVEL = 1
DEBUG_LEVEL = 3
LOG_LEVEL = 7
LOG_FILE = kfs.log

#define LOG_EMERG   0
#define LOG_ALERT   1
#define LOG_CRIT    2
#define LOG_ERR     3
#define LOG_WARNING 4
#define LOG_NOTICE  5
#define LOG_INFO    6
#define LOG_DEBUG   7

# LOG_FILE = /dev/stdout
CFLAGS += -DLOG_LEVEL=$(LOG_LEVEL) -O$(OPTIM_LEVEL)

ifeq ($(MAKECMDGOALS),debug)
CFLAGS+=-g$(DEBUG_LEVEL)
OPTIM_LEVEL = 0
endif

KFSCTL_SRC_PATH = kfsctl
KFSCTL = kfsctl

all: $(SRC_PATH)/$(TARGET) $(MKFS_SRC_PATH)/$(MKFS) $(KFSCTL_SRC_PATH)/$(KFSCTL)

debug: all

$(SRC_PATH)/$(TARGET): $(OBJ)
	$(CC) $^ $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# ------------------------- #
#          使用方法
# ------------------------- #
.PHONY: clean distclean lib release tar all test kfsd

# make : 编译
# make clean: 清除编译的中间文件
# make distclean: 清除所有编译结果
# make lib: 将所有obj整合到一个.a
# make release: 导出release库
# make tar: 打包release
# make install: 安装release库
# make uninstall: 卸载release库

# Define variables for formatting
CP_FORMAT = "[cp]\t%-20s -> %s\n"
MV_FORMAT = "[mv]\t%-20s -> %s\n"

DISK_IMG = disk.img

disk:
	dd if=/dev/zero of=$(DISK_IMG) bs=1M count=1000
	$(MAKE) reset

reset:
	$(MKFS_SRC_PATH)/$(MKFS) $(DISK_IMG)

run:
	@$(SRC_PATH)/$(TARGET) $(DISK_IMG) $(TMP_PATH) -o logfile=$(LOG_FILE)
	@echo "mount fs in $(TMP_PATH), log file save in: $(LOG_FILE)"

debug_run:
	@$(SRC_PATH)/$(TARGET) $(DISK_IMG) $(TMP_PATH) -d -o logfile=$(LOG_FILE)

um:
	umount -l $(TMP_PATH)

mkfs: $(MKFS_SRC_PATH)/$(MKFS)

MKFS_DEPENDS = $(SRC_PATH)/disk.o $(SRC_PATH)/logging.o

$(MKFS_SRC_PATH)/$(MKFS): $(MKFS_OBJ) $(MKFS_DEPENDS)
	$(CC) $^ $(LDFLAGS) -o $@

$(MKFS_OBJ): $(MKFS_SRC)
	$(CC) $(CFLAGS) -I$(SRC_PATH) -c $< -o $@

$(KFSCTL_SRC_PATH)/$(KFSCTL): $(SRC) $(KFSCTL_SRC_PATH)/*.c
	$(MAKE) -C $(KFSCTL_SRC_PATH)

SHARED_DIR = /etc

kfsd:
	cd kfsd && cargo build --release && cargo run -- --shared-dir $(SHARED_DIR) --socket-path /tmp/vhostqemu --cache auto

dump:
	$(DUMPFS) -f $(DISK_IMG)

test:
	@$(MAKE) disk > /dev/null
	@$(MAKE) run > /dev/null
	cd $(TMP_PATH) && ../$(TEST_PATH)/test_run.sh
	@$(MAKE) um > /dev/null

clean:
	rm -f $(OBJ) $(MKFS_OBJ) $(SRC_PATH)/$(TARGET) $(MKFS_SRC_PATH)/$(MKFS)
	$(MAKE) -C $(KFSCTL_SRC_PATH) clean

release:
	$(MAKE) -j4
	mkdir $(RELEASE)
	@cp $(EXE) $(RELEASE)/ 
	tar -cvf $(TARGET).tar $(RELEASE)/

c:
	cd tmp && ../$(KFSCTL_SRC_PATH)/kfsctl status

help:
	@echo -e ""
	@echo -e "\tmake \t\t\t编译"
	@echo -e "\tmake debug \t\t编译debug版本"
	@echo -e "\tmake test \t\t测试"
	@echo -e "\tmake clean \t\t清除编译的中间文件"
	@echo -e "\tmake distclean \t\t清除所有编译结果"
	@echo -e "\tmake lib \t\t将所有obj整合到一个.a"
	@echo -e "\tmake release \t\t导出release库"
	@echo -e "\tmake tar \t\t打包release"
	@echo -e "\tmake install \t\t安装release库"
	@echo -e "\tmake uninstall \t\t卸载release库"
	@echo -e "\tmake help \t\t显示帮助信息"
	@echo -e ""
# -Wshadow 局部变量和全局变量同名
# -Wpedantic 严格的ISO C 标准警告
# -Wstrict-aliasing 警告可能会引发严格别名规则的别名问题
# -Wunnitialized 未初始化局部变量
# -Wnull-dereference 警告可能的空指针解引用
# -Wformat=2 字符串格式化
# -Wformat-security 警告可能会引发格式字符串安全问题
