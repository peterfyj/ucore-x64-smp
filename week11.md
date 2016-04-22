# Ucore part #

## Work done ##

  * User Mode ucore fully merged and tested.

## TODO ##

  * Review the code of page management and decide how much of it can be regarded as architecture independent.
  * Merge OpenRisc 1200 implementation.


---


# Go part #

## Work done ##

  * Disable cgo by setting the environment variable CGO\_ENABLED;
  * Port some syscalls, including:
    * signalstack (direct return),
    * exit (ported & untested),
    * exit1 (ported & untested),
    * sigreturn (direct return),
    * mmap (ported & untested),
    * munmap (ported & untested),
    * sem\_init, sem\_post, sem\_wait, sem\_free (ported & untested),
    * `SysAlloc` (ported & untested),
    * `SysReserve` (ported, which will not work on 64-bit machine),
    * `SysMap` (ported & untested),
    * lock, unlock, usemacquire (ported & untested),
    * notewakeup, notesleep (ported & untested),
    * goenvs (direct return);

## Problems remain ##

The reason for not testing ported syscalls is that current ucore has no segment management, which leads all cpus to share the same gdt.  This gains the difficulty to implement tls, and the gdt is likely to be full...

gdt\_pd has a 16-bit field indicating the length of gdt, so that the maximum number of gdt items would reach 216 / 26 = 1024.  We now have 6 + 256 + 256 `*` 2 = 774...

In addition, the 64-bit go does not reserve memory with `SysReserve` and the new strategy is to be tested.