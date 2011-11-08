.PHONY: all clean qemu

-include config.mk

export V        		?= @
export T_BASE   		?= ${PWD}
export T_OBJ    		?= ${PWD}/obj
export MAKE     		:= make -s
QEMU            		?= qemu-system-x86_64

export HOST_CC_PREFIX	?=
export TARGET_CC_PREFIX	?=
export TARGET_CC_FLAGS_COMMON	?=	-ffreestanding \
									-mno-red-zone \
									-mno-mmx -mno-sse -mno-sse2 \
									-mno-sse3 -mno-3dnow \
									-fno-builtin -fno-builtin-function -nostdinc
export TARGET_CC_FLAGS_BL		?=	-m32
export TARGET_CC_FLAGS_KERNEL	?=	-m64 -mcmodel=kernel
export TARGET_CC_FLAGS_SV		?=	-m64 -mcmodel=large
export TARGET_CC_FLAGS_USER		?=	-m64 -mcmodel=small
export TARGET_LD_FLAGS			?=	-m $(shell ld -V | grep elf_i386 2>/dev/null) -nostdlib

export OBJDUMP			?= objdump
export OBJCOPY 			?= objcopy
export STRIP   			?= strip

export BRANCH   		?= ucore

HAS_DRIVER_OS   		?= n
ifneq (${HAS_DRIVER_OS},n)
override HAS_DRIVER_OS := y
endif
export HAS_DRIVER_OS

ifeq (${BRANCH},linux-dos-module)
all: ${T_OBJ}
	${V}${MAKE} -f mk/mods.mk all
else
all: ${T_OBJ}
	${V}${MAKE} -f mk/mods.mk all
	${V}${MAKE} -f mk/image.mk all
endif

${T_OBJ}:
	@mkdir -p ${T_OBJ}

ifeq (${BRANCH},linux-dos-module)
clean:
	${V}${MAKE} -C src/linux-dos-module clean
	-${V}rm -rf ${T_OBJ}/dosm.ko
else
clean:
	-${V}rm -rf ${T_OBJ}/*
endif

qemu: all
	${QEMU} -smp 4 -m 512 \
	-hda ${T_OBJ}/kernel.img \
	-drive file=${T_OBJ}/swap.img,media=disk,cache=writeback \
	-drive file=${T_OBJ}/sfs.img,media=disk,cache=writeback \
	-s -S \
	-serial file:${T_OBJ}/serial.log -monitor stdio
