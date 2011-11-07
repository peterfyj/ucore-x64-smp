.PHONY: all

SRCFILES   := $(shell find . '(' '!' -regex '.*/_.*' ')' -and '(' -iname "*.c" -or -iname "*.S" ')' | sed -e 's!\./!!g') ../glue-kern/bootinfo.S
T_CC_FLAGS := 	-I../glue-kern -Iglue-ucore -Iglue-ucore/libs -Ilibs -Idebug -Idriver -Imm -Isync -Ischedule -Iprocess -Ifs \
				-Ifs/swap -Ifs/vfs -Ifs/pipe -Ifs/sfs -Ifs/devs -Isyscall

include ${T_BASE}/mk/compk.mk
include ${T_BASE}/mk/template.mk

all: ${T_OBJ}/${KERN_FILELIST}

${T_OBJ}/${KERN_FILELIST}: ${OBJFILES}
	@echo LD $@
	${V}echo ${OBJFILES} >  ${T_OBJ}/${KERN_FILELIST}
