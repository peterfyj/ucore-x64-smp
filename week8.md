# Ucore part #
## Introduction ##

Make it possible for several threads of one process to run on different CPUs at the same time. Work on branch **MT-on-MP-eternalNight**. Not merged to **master** yet.

## Details ##

### Modifications in the kernel ###
  * Instead of inserting an entry in the page table, one more element in the **lcpu\_static** array, which is the base address of an allocated page for PLS, is maintained by **supervisor**. When executing the kernel code, each cpu can ask for its own PLS page by invoking **pls\_base\_get**.
  * Instead of directly accessing the PLS section variables, a proper macro has to be used. There're three macros at present.
    * **pls\_read(var)** - read the value of variable pls`_`##var.
    * **pls\_write(var,value)** - write value to variable pls`_`##var.
    * **pls\_get\_ptr(var)** - get a pointer to variable pls`_`##var.
  * Add some information-collecting code. When debuging with gdb, some functions can be called to print some information on scheduling, including what processes are recently running (**db\_sched**), or how many time slices are allocated for processors whose PIDs fall into a certain range(**db\_time**). Implemented in ucore/src/kern-ucore/sched/sched.c.

### Further work ###
  * Use segmentation to access the PLS page.


---


# Go part #

## Work done ##

  * Update go language source code to `r60` version;
  * Go configured for cross-compiling.  Now, four environment variables will be used: GOOS, GOARCH, GOHOSTOS， GOHOSTARCH, separately naming the operating system and architecture of the target and the host.  The testing procedure will be ignored so as to compile quickly;

## Questions remain ##

  * No assembly output for 6.out (the binary file for hello world), which disables further debugging;
  * Unable to change the syscall interrupt number.  What's now in the source code of go is merely an assembly instruction 'SYSCALL', which confuses me;
  * Running 6.out directly will end up with 'Invalid parameter';