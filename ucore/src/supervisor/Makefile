.PHONY: all

SRCFILES   := $(shell find . '(' '!' -regex '.*/_.*' ')' -and '(' -iname "*.c" -or -iname "*.S" ')' | sed -e 's!\./!!g')
T_CC_FLAGS := -I../libs-sv -Idebug -Idriver -Iinit -Ilibs -Imm -Itrap -Isync -Imp -Icontext

include ${T_BASE}/mk/compsv.mk
include ${T_BASE}/mk/template.mk

ifeq (${HAS_DRIVER_OS},y)
OBJFILES   += ${T_OBJ}/bzImage.o
T_CC_FLAGS += -DHAS_DRIVER_OS=1
endif

all: ${T_OBJ}/supervisor ${T_OBJ}/kern-svsym.S.o ${T_OBJ}/kern-bin

${T_OBJ}/supervisor: ${OBJFILES} supervisor.ld
	@echo LD $@
	${V}${LD} -T supervisor.ld -z max-page-size=0x1000 -o$@ ${OBJFILES$} ${T_OBJ}/libs-sv-*.o 

${T_OBJ}/svsym.o: ${T_OBJ}/supervisor
	${V}objcopy --extract-symbol $< $@
	${V}strip -x $@

${T_OBJ}/kern-svsym.S: ${T_OBJ}/svsym.o
	@echo GEN $@
	${V}echo ".data" > $@
	${V}echo ".align 8" >> $@
	${V}objdump -t $< -j .text -j .rodata -j .data -j .bss \
		| sed -n -e 's/^\([0-9a-f][0-9a-f]*\).* \([a-zA-Z0-9_][a-zA-Z0-9_]*\)$$/\2_ptr: .quad 0x\1/gp' \
		| sed -e 's/__TO_EXPORT_\([a-zA-Z0-9_][a-zA-Z0-9_]*\).*/.global \1/g' >> $@

${T_OBJ}/kern-svsym.S.o: ${T_OBJ}/kern-svsym.S
	@echo CC $@
	@${CC} ${CC_ALL_FLAGS} $< -c -o$@
	@strip -x $@

${T_OBJ}/kern-bin: ${T_OBJ}/kern
	@echo OBJCOPY $@
	${OBJCOPY} -S -O binary $^ $@
	@T_OBJ=${T_OBJ} sh ${T_BASE}/misc/kerninfo.sh

${T_OBJ}/kern: ${T_OBJ}/kern-*.o ../glue-kern/kern.ld
	@echo LD $@
	${V}${LD} -T ../glue-kern/kern.ld -z max-page-size=0x1000 -o$@ ${T_OBJ}/kern-*.o
