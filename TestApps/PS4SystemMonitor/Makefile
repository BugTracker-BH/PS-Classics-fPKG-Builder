ifndef OO_PS4_TOOLCHAIN
    $(error OO_PS4_TOOLCHAIN is not set. Please set it to the OpenOrbis SDK path.)
endif

TARGET     := eboot.bin
FINALDIR   := app

SRCDIR     := src
OBJDIR     := obj

CC         := $(OO_PS4_TOOLCHAIN)/bin/linux/x86_64/clang
LD         := $(OO_PS4_TOOLCHAIN)/bin/linux/x86_64/ld.lld
OBJCOPY    := $(OO_PS4_TOOLCHAIN)/bin/linux/x86_64/llvm-objcopy
CREATE_EBOOT := python3 $(OO_PS4_TOOLCHAIN)/bin/create-eboot.py

SFLAGS     := -cc1 -triple x86_64-scei-ps4-elf -munwind-tables
CFLAGS     := --target=x86_64-scei-ps4-elf \
              -DORBIS -D__PS4__ \
              -isystem $(OO_PS4_TOOLCHAIN)/include \
              -isystem $(OO_PS4_TOOLCHAIN)/include/c++/v1 \
              -Wall -O2 -fPIC -fno-builtin

LDFLAGS    := -m elf_x86_64 \
              -pie --script $(OO_PS4_TOOLCHAIN)/link.x \
              --eh-frame-hdr \
              -L$(OO_PS4_TOOLCHAIN)/lib \
              -lc -lkernel -lSceVideoOut -lScePad -lSceSysmodule -lSceUserService

SRCS       := $(wildcard $(SRCDIR)/*.c)
OBJS       := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

.PHONY: all clean pkg

all: $(FINALDIR)/$(TARGET)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(FINALDIR):
	mkdir -p $(FINALDIR)

$(OBJDIR)/$(TARGET:.bin=.elf): $(OBJS)
	$(LD) $(OBJS) $(LDFLAGS) -o $@

$(FINALDIR)/$(TARGET): $(OBJDIR)/$(TARGET:.bin=.elf) | $(FINALDIR)
	$(CREATE_EBOOT) --paid 0x3800000000000011 $< $@

clean:
	rm -rf $(OBJDIR) $(FINALDIR)/$(TARGET)

pkg: all
	@echo "=== eboot.bin ready at $(FINALDIR)/$(TARGET) ==="
	@echo "=== Use PS Classics fPKG Builder (PS4 App tab) to compile PKG ==="
	@echo "=== Point App Folder to: $(FINALDIR)/ ==="
