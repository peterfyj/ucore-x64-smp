SRCFILES	+= $(shell find . '(' '!' -regex '.*/_.*' ')' -and '(' -iname "*.c" -or -iname "*.S" ')' | sed -e 's!\./!!g')
ARCH_DIRS	:= debug driver process glue-ucore glue-ucore/libs
T_CC_FLAGS	+= ${foreach dir,${ARCH_DIRS},-Iarch/${ARCH}/${dir}}

include ${T_BASE}/mk/compk.mk
include ${T_BASE}/mk/template.mk

all: ${OBJFILES}
