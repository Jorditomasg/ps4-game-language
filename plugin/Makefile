# Requires OO_PS4_TOOLCHAIN (OpenOrbis toolchain) and GOLDHEN_SDK (built GoldHEN_Plugins_SDK).
NAME      := game_language
TOOLCHAIN := $(OO_PS4_TOOLCHAIN)
GH_SDK    := $(GOLDHEN_SDK)
BUILD     := build
SRCS      := $(wildcard source/*.c) ../common/langcfg.c
OBJS      := $(patsubst %.c,$(BUILD)/%.o,$(notdir $(SRCS)))
vpath %.c source ../common

LIBS    := -lSceLibcInternal -lkernel -lSceSysmodule -lSceSystemService -lGoldHEN_Hook
CFLAGS  := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -c -Wall -O2 \
           -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include -I$(GH_SDK)/include -I../common
LDFLAGS := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x -e _init --eh-frame-hdr \
           -L$(TOOLCHAIN)/lib -L$(GH_SDK) $(LIBS)

.PHONY: all clean

all: bin/$(NAME).prx

bin/$(NAME).prx: $(OBJS) | bin
	ld.lld $(GH_SDK)/build/crtprx.o $(OBJS) -o $(BUILD)/$(NAME).elf $(LDFLAGS)
	$(TOOLCHAIN)/bin/linux/create-fself -in=$(BUILD)/$(NAME).elf -out=$(BUILD)/$(NAME).oelf \
		--lib=$@ --paid 0x3800000000000011

$(BUILD)/%.o: %.c | $(BUILD)
	clang $(CFLAGS) -o $@ $<

$(BUILD) bin:
	mkdir -p $@

clean:
	rm -rf $(BUILD) bin
