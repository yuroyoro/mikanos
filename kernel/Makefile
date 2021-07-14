TARGET = kernel.elf
OBJS = main.o graphics.o font.o hankaku.o newlib_support.o console.o
DEPENDS = $(join $(dir $(OBJS)), $(addprefix ., $(notdir $(OBJS:.o=.d))))

# set build env, copied from ~/osbook/devenv/buildenv.sh
EDK2_DIR   = $(HOME)/edk2
OSBOOK_DIR = $(HOME)/osbook
X86_64_ELF_DIR = $(OSBOOK_DIR)/devenv/x86_64-elf

CFLAGS += -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone
CPPFLAGS = -I$(X86_64_ELF_DIR)/include/c++/v1 -I$(X86_64_ELF_DIR)/include \
           -I$(X86_64_ELF_DIR)/include/freetype2 \
		   -I$(EDK2_DIR)/MdePkg/Include -I$(EDK2_DIR)/MdePkg/Include/X64 \
		   -nostdlibinc -D__ELF__ -D_LDBL_EQ_DBL -D_GNU_SOURCE -D_POSIX_TIMERS \
		   -DEFIAPI='__attribute__((ms_abi))'

CXXFLAGS += -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone \
            -fno-exceptions -fno-rtti -std=c++17
LDFLAGS = -L$(X86_64_ELF_DIR)/lib
LDFLAGS += --entry KernelMain -z norelro --image-base 0x100000 --static

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	rm -rf *.o .*.d $(TARGET)

kernel.elf: $(OBJS) Makefile
	ld.lld $(LDFLAGS) -o kernel.elf $(OBJS) -lc

%.o: %.cpp Makefile
	clang++ $(CPPFLAGS) $(CXXFLAGS) -c $<

%.o: %.c Makefile
	clang $(CPPFLAGS) $(CFLAGS) -c $<

.%.d: %.cpp
	clang++ $(CPPFLAGS) $(CXXFLAGS) -MM $< > $@
	$(eval OBJ = $(<:.cpp=.o))
	sed --in-place 's|$(notdir $(OBJ))|$(OBJ)|' $@

.%.d: %.c
	clang $(CPPFLAGS) $(CFLAGS) -MM $< > $@
	$(eval OBJ = $(<:.c=.o))
	sed --in-place 's|$(notdir $(OBJ))|$(OBJ)|' $@

hankaku.bin: hankaku.txt
	../tools/makefont.py -o $@ $<

hankaku.o: hankaku.bin
	objcopy -I binary -O elf64-x86-64 -B i386:x86-64 $< $@

.%.d: %.bin
	touch $@

.PHONY: depends
depends:
	$(MAKE) $(DEPENDS)

-include $(DEPENDS)