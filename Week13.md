# Ucore part #



# Go part #

## Work done ##

  * **Simple hello world done.**  The problem described last week is due to argc and argv, which are not pushed to stack by ucore.  Manually push them in `_`rt0`_`ucoresmp in the runtime package of go;
  * Bit 9 of cr4 is set to 1 so as to enable fpu instructions.  **NOTE: No registers are set!  Fpu may not work correctly!**
  * Cancel the virtual map of syscalls such as gettimeofday(): ucore does not support this.

## TODO ##

  * Now a single program takes about 40 seconds to load.  Much further is to be researched;
  * fmt package: advanced hello world;
  * OS-level thread;