# Ucore part #


---


# Go part #

## Questions remain ##
  * x86-64 syscalls differ from those of x86;
  * Tls mechanism on x86-64 differ from that of x86 (arch\_prctl instead of modify modify\_ldt), but still need new implementation;

## Work to be done ##
  * Implement 'runtime' package referring to the implementation on x86 port;