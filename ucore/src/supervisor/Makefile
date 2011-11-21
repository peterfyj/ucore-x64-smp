.PHONY: all

SRCFILES	:= $(filter %.c %.S, $(wildcard */*))
T_CC_FLAGS	:= -I../glue-kern -Idebug -Idriver -Iinit -Ilibs -Imm -Itrap -Isync -Imp -Icontext

include arch/$(ARCH)/supervisor.mk
