SRCFILES	+= $(filter %.c %.S, $(wildcard arch/${ARCH}/*))
T_CC_FLAGS	+= -Iarch/${ARCH}

include ${T_BASE}/mk/compu.mk
include ${T_BASE}/mk/template.mk

all: ${OBJFILES}
