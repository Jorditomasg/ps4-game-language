# Requires OO_PS4_TOOLCHAIN (OpenOrbis toolchain) and the plugin built in ../plugin/bin.
TITLE      := Game Language
VERSION    := 01.00
TITLE_ID   := GLNG00001
CONTENT_ID := IV0000-GLNG00001_00-GAMELANGUAGE0000

TOOLCHAIN := $(OO_PS4_TOOLCHAIN)
TOOLS     := $(TOOLCHAIN)/bin/linux
SAMPLE    := $(TOOLCHAIN)/samples/hello_world
BUILD     := build
SRCS      := $(wildcard source/*.c) ../common/langcfg.c ../common/plugins_ini.c
OBJS      := $(patsubst %.c,$(BUILD)/%.o,$(notdir $(SRCS)))
vpath %.c source ../common

LIBS    := -lc -lkernel -lSceVideoOut -lScePad -lSceUserService -lSceSystemService -lSceSysmodule
CFLAGS  := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -c -Wall -O2 \
           -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include -I../common -Ithird_party
LDFLAGS := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr -L$(TOOLCHAIN)/lib $(LIBS) \
           $(TOOLCHAIN)/lib/crt1.o

PKG_FILES := eboot.bin sce_sys/param.sfo sce_sys/icon0.png sce_sys/about/right.sprx \
             sce_module/libc.prx sce_module/libSceFios2.prx assets/game_language.prx \
             assets/fonts/NotoSans-Regular.ttf assets/fonts/NotoSans-SemiBold.ttf

.PHONY: all clean

all: $(CONTENT_ID).pkg

$(CONTENT_ID).pkg: pkg.gp4
	$(TOOLS)/PkgTool.Core pkg_build $< .

pkg.gp4: $(PKG_FILES)
	$(TOOLS)/create-gp4 -out $@ --content-id=$(CONTENT_ID) --files "$^"

eboot.bin: $(OBJS)
	ld.lld $(OBJS) -o $(BUILD)/app.elf $(LDFLAGS)
	$(TOOLS)/create-fself -in=$(BUILD)/app.elf -out=$(BUILD)/app.oelf --eboot $@ --paid 0x3800000000000011

$(BUILD)/%.o: %.c | $(BUILD)
	clang $(CFLAGS) -o $@ $<

$(BUILD):
	mkdir -p $@

assets/game_language.prx: ../plugin/bin/game_language.prx
	cp $< $@

# Runtime modules shipped with the OpenOrbis samples.
sce_module/%.prx:
	mkdir -p sce_module && cp $(SAMPLE)/sce_module/$*.prx $@

sce_sys/about/right.sprx:
	mkdir -p sce_sys/about && cp $(SAMPLE)/sce_sys/about/right.sprx $@

sce_sys/param.sfo: Makefile
	$(TOOLS)/PkgTool.Core sfo_new $@
	$(TOOLS)/PkgTool.Core sfo_setentry $@ APP_TYPE --type Integer --maxsize 4 --value 1
	$(TOOLS)/PkgTool.Core sfo_setentry $@ APP_VER --type Utf8 --maxsize 8 --value '$(VERSION)'
	$(TOOLS)/PkgTool.Core sfo_setentry $@ ATTRIBUTE --type Integer --maxsize 4 --value 0
	$(TOOLS)/PkgTool.Core sfo_setentry $@ CATEGORY --type Utf8 --maxsize 4 --value 'gd'
	$(TOOLS)/PkgTool.Core sfo_setentry $@ CONTENT_ID --type Utf8 --maxsize 48 --value '$(CONTENT_ID)'
	$(TOOLS)/PkgTool.Core sfo_setentry $@ DOWNLOAD_DATA_SIZE --type Integer --maxsize 4 --value 0
	$(TOOLS)/PkgTool.Core sfo_setentry $@ SYSTEM_VER --type Integer --maxsize 4 --value 0
	$(TOOLS)/PkgTool.Core sfo_setentry $@ TITLE --type Utf8 --maxsize 128 --value '$(TITLE)'
	$(TOOLS)/PkgTool.Core sfo_setentry $@ TITLE_ID --type Utf8 --maxsize 12 --value '$(TITLE_ID)'
	$(TOOLS)/PkgTool.Core sfo_setentry $@ VERSION --type Utf8 --maxsize 8 --value '$(VERSION)'

clean:
	rm -rf $(BUILD) eboot.bin pkg.gp4 *.pkg sce_sys/param.sfo sce_sys/about sce_module assets/game_language.prx
