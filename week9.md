# Ucore part #

## Work done ##

Page table accessors are added and all occurences of directly accessing to page table entries are replaced with these inline functions. Duplicated headers and function implementations are partly dealt with.

## TODO ##

Continue merging duplicated things. Move all drivers down to supervisor (or regard them as another part? A closer look is needed.)


# Go part #

## Work done ##

  * GDB recompiled for debugging;
  * objdump used for assembly code analysis;
  * Problem of 'Invalid parameter' is due to USER\_BASE, which is 0x400000 in linux but 0x1000000 in Ucore.  The binary could be run after changing the go linker behavior towards USER\_BASE;
  * Problem of 'SYSCALL' is actually not anything confusing.  IT IS AN INSTRUCTION.  Reference could be found here: [SYSCALL](http://www.codemachine.com/article_syscall.html).  However, this new method requires the system register all syscalls to CPU during starting up, which is time-consuming for the moment.  I've decided to use the original way: INT 0x80;

## Problems remain ##

  * Slow in ucore starting up: XXX fix?
  * Slow in 6.out (binary file for hello world) loading: one header sized about 32MB costs half a minute for page allocation;
  * TLS?  Waiting for another branch.