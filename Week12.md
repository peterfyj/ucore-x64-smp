# Ucore part #

## Work done ##
**Or32 and i386 implementations merged.**

All implementations can work now, passing all memory management checks, starting the shell and execute several binaries like hello, sem\_rf and threadwork. After executing several programs, the shell quits and the post-shell memory constraints are held on i386, x86\_64 and um. (In the current version of simulator for or32, it is not possible to input special characters like Ctrl-C...)

**Compile ulib to a static library.**

Statically linked user binaries are always smaller than the original object-linked ones because unused symbols are all eliminated.

**Fix load\_icode.**

Sections whose filesz is 0 should be still mapped when loading the binary, initialzing with 0.

In the previous revision, those 'empty' sections are totally skipped. Thanks to a static uint64\_t value in rand.c, empty sections never occur because there will always be at least 8 bytes in data&bss section.

However, if the binaries are statically-linked with a pre-built library, the unused value is removed and the data&bss section remains nothing in the elf file.

## TODO ##

**Full test**

Test all the implementations thoroughly using all existing binaries. How can it be done automatically?

**Documentation**

Review the current HAL and write documents on it.


---


# Go part #

## Work done ##
  * Now every CPU has its own copy of gdt, with two entries for fs & gs as tls.  The newly-implemented syscall sys\_prctl() is used to set the fs & gs base;
  * The memory allocation in x86\_64 differs from that of x86, which does not reserve memory in SysReserve().  It doesn't matter if the new way is implemented logically.

## Problem ##
  * Unknown pagefault...  _See week 13 report_