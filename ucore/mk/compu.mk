KPREFIX	:= x86_64-elf-

CC		:= ${KPREFIX}gcc -m64 -ffreestanding \
			-mcmodel=small -mno-red-zone \
			-mno-mmx -mno-sse -mno-sse2 \
			-mno-sse3 -mno-3dnow \
			-fno-builtin -fno-builtin-function -nostdinc

LD		:=  ${KPREFIX}ld -m $(shell ${KPREFIX}ld -V | grep elf_x86_64 2>/dev/null) -nostdlib
OBJCOPY :=  ${KPREFIX}objcopy

include ${T_BASE}/mk/compopt.mk
