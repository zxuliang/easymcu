# common makefile for easymcu



SRCS_C +=proj/fwlib/cmsis/core_cm3.c
SRCS_C +=$(wildcard proj/fwlib/stmstd/src/*.c)
SRCS_S +=proj/fwlib/startup.S

INCLUDES += -Iproj/fwlib/cmsis
INCLUDES += -Iproj/fwlib/stmstd/inc

OBJS :=$(patsubst proj/%.c, out/%.o, $(SRCS_C))
OBJS +=$(patsubst proj/%.S, out/%.o, $(SRCS_S))
DEPS :=$(OBJS:.o=.d)

#common
USE_NANOLIB = --specs=nano.specs
NOSEMIHOST = --specs=nosys.specs
ARCHFLAGS := -march=armv7-m -mcpu=cortex-m3 -mthumb

# define soc and using fwlibs
CFLAGS += -DSTM32F10X_HD
CFLAGS += -DUSE_STDPERIPH_DRIVER
CFLAGS += $(APPCFLAGS)

CFLAGS += $(ARCHFLAGS)
CFLAGS += -std=gnu99 -Wall -O0 -g
CFLAGS += -fno-common -fno-builtin
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += $(USE_NANOLIB)

AFLAGS += -fomit-frame-pointer -fno-common -nostdinc
AFLAGS += $(USE_NANOLIB) $(ARCHFLAGS)

####
#### pass linker options -Wl,--gc-sections using gcc
#### pass linker options --gc-sections using ld
#### @$(LD) $(OBJS) $(LDFLAGS) -o $@ -Map $(basename $@).map
#### -Wl,--start-group -lgcc -lc -lnosys -Wl,--end-group
####
LINKER_SCRIPT = proj/fwlib/proj.ld
LDFLAGS += $(USE_NANO LIB) $(NOSEMIHOST)
LDFLAGS += -nostartfiles -nostdlib
LDFLAGS +=  -Wl,--gc-sections -Wl,--build-id=none -Bstatic
LDFLAGS += -T$(LINKER_SCRIPT)

CROSS_COMPILE =arm-none-eabi-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
NM = $(CROSS_COMPILE)nm
OBJDUMP = $(CROSS_COMPILE)objdump
OBJCOPY = $(CROSS_COMPILE)objcopy
SIZE = $(CROSS_COMPILE)size

OUTPUT=out
TARGET:=${OUTPUT}/easymcu.elf

.PHONY: all clean

all: ${TARGET}
	@echo "--------------------------------------------------"
	@$(SIZE) $<

${OUTPUT}/%.elf: $(OBJS)
	@echo ""
	@echo "CC -Wl,--start-group ${OBJS} -Wl,--end-group $(LDFLAGS) -o $@"
	@$(CC) -Wl,--start-group $(OBJS) -Wl,--end-group $(LDFLAGS) -o $@
	@$(OBJCOPY) --gap-fill=0xff -O binary  -R .note -R .comment -S $@ $(subst .elf,.bin,$@)
	@$(OBJCOPY) --gap-fill=0xff -O ihex -R .note -R .comment -S $@ $(subst .elf,.hex,$@)
	@$(NM) -n $@ | grep -v '\( [aUw] \)|\(__crc__\)' > $(subst .elf,.map,$@)

${OUTPUT}/%.o: proj/%.c
	@mkdir -p $(dir $@)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

${OUTPUT}/%.o: proj/%.S
	@mkdir -p $(dir $@)
	@echo "CC  $<"
	@$(CC) $(AFLAGS) $(INCLUDES) -c $< -o $@

#Auto depends for c;
${OUTPUT}/%.d:proj/%.c
	@mkdir -p ${dir $@}
	@set -e; rm -f $@; $(CC) -MM $< $(INCLUDES) $(CFLAGS) > $@.$$$$;\
	sed 's,\(.*\)\.o[ :]*,$(addprefix $(dir $@),\1.o):,g' < $@.$$$$ > $@;\
	rm -f $@.$$$$

# Auto depends for .S too
${OUTPUT}/%.d:proj/%.S
	@mkdir -p ${dir $@}
	@set -e; rm -f $@; $(CC) -MM $< $(INCLUDES) $(CFLAGS) > $@.$$$$;\
	sed 's,\(.*\)\.o[ :]*,$(addprefix $(dir $@),\1.o):,g' < $@.$$$$ > $@;\
	rm -f $@.$$$$

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif

#Tell makefile dont't autoremove anything
.SECONDARY:

clean:
	@-rm -rf ${OUTPUT}
	@echo "Cleaning...done"
