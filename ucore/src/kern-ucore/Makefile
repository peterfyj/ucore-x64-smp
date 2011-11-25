.PHONY: all

SRCFILES   := $(shell find . '(' '!' -regex '.*/_.*' ')' -and '(' -iname "*.c" -or -iname "*.S" ')' -and -not -regex '\./arch.*' | sed -e 's!\./!!g') ../glue-kern/bootinfo.S
T_CC_FLAGS := -I../glue-kern -Iglue-ucore -Iglue-ucore/libs -Ilibs -Idebug -Idriver -Imm -Isync -Ischedule -Iprocess -Ifs \
			  -Ifs/swap -Ifs/vfs -Ifs/pipe -Ifs/sfs -Ifs/devs -Isyscall

include arch/$(ARCH)/ucore.mk


