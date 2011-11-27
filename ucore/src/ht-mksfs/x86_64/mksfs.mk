.PHONY: all

SRCFILES   := ${ARCH}/mksfs.c
T_CC_FLAGS := -Wall -O2 -D_FILE_OFFSET_BITS=64

include ${T_BASE}/mk/comph.mk
include ${T_BASE}/mk/template.mk

all: ${T_OBJ}/tools-mksfs

${T_OBJ}/tools-mksfs: ${OBJFILES}
	@echo LD $@
	${V}${CC} -o$@ $^
