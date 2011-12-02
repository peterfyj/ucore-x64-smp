export HOST_CC_PREFIX	?=
export TARGET_CC_PREFIX	?= or32-elf-
export TARGET_CC_FLAGS_COMMON	?= -fno-builtin -nostdinc -fno-stack-protector -mhard-mul
export TARGET_CC_FLAGS_BL		?=
export TARGET_CC_FLAGS_KERNEL	?=
export TARGET_CC_FLAGS_SV		?=
export TARGET_CC_FLAGS_USER		?=
export TARGET_LD_FLAGS			?= -nostdlib

BOOTLOADER	:= ${T_OBJ}/bootloader
KERNEL		:= ${T_OBJ}/kernel
SWAPIMG		:= ${T_OBJ}/swap.img
FSIMG		:= ${T_OBJ}/sfs.img
MEM 		:= 128M
BOOT_OPTS	:= --kernel=$(KERNEL) --swap=$(SWAPIMG) --memsize=$(MEM) --disk=$(FSIMG)

run: all
	${V}${BOOTLOADER} $(BOOT_OPTS)

debug: all
	${V}/usr/bin/gdb -q -x gdbinit.${ARCH}
