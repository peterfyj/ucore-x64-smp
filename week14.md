# Ucore part #


---


# Go part #

## Work done ##

  * **Advanced hello world (see below) done**
  * **peter.go (see below) done**

## Implementation ##

  1. Bit 9 of cr4 is set to 1 to enable sse2;
  1. Qemu is set to launch with sse2 support;
  1. Lock strategy: derived from that of the 32-bit go porting.  With the support of sem\_init(), sem\_post(), sem\_wait() and sem\_free(),  higher-level functions are implemented in the os-dependent level of go source, including runtime.lock(), runtime.unlock(), usemacquire(), usemrelease().
  1. Basic modifications on the syscall package: exit, write, time, etc;
  1. Multi-thread: clone() implemented, though with some difficulty (see below);

## Details ##

While trying to make peter.go running, I encountered a problem.  After the call to clone(), the new thread would always generate a page fault.  Suspicion is first put upon the fs base, but it turns out that the lower 32 bits of the base are working while the higher ones  are not.

The answer to this is found in the ia-32e manual:

_In 64-bit mode, memory accesses using FS-segment and GS-segment overrides are not checked for a runtime limit nor subjected to attribute-checking. Normal segment loads (MOV to Sreg and POP Sreg) into FS and GS load a standard 32-bit base value in the hidden portion of the segment descriptor register. The base address bits above the standard 32 bits are cleared to 0 to allow consistency for implementations that use less than 64 bits._

_The hidden descriptor register fields for FS.base and GS.base are physically mapped to MSRs in order to load all address bits supported by a 64-bit implementation. Soft-ware with CPL = 0 (privileged software) can load all supported linear-address bits into FS.base or GS.base using WRMSR. Addresses written into the 64-bit FS.base and GS.base registers must be in canonical form. A WRMSR instruction that attempts to write a non-canonical address to those registers causes a #GP fault._

It indicates two things:
  1. When the bases of fs / gs need to be changed, WRMSR should be used instead of changing the gdt;
  1. After calling WRMSR, fs / gs should NOT be written before returning to the user's code.  Our solution is to set fs / gs in trapframe in proc\_run(), and cancel the stack restore of fs / gs;

## Appendix ##

hw2.go: The simple "hello world", which needs only the runtime package;
```
package main

func main() {
	print("Hello, world!\n")
}
```

hw1.go: The advanced "hello world", which needs the fmt package and its depending packages such as syscall package;
```
package main

import "fmt"

func main() {
	fmt.Println("Hello, World!")
}
```

peter.go: Goroutines each representing a system level thread;
```
package main

import (
	"fmt"
	"time"
)

var c chan int

func ready(index int) {
	time.Sleep(int64(index) * 1e9)
	fmt.Println(index)
	c<-1
}

func main() {
	total := 10
	c = make(chan int)
	for i := 0; i < total; i++ {
		go ready(i + 1)
	}
	for i := 0; i < total; i++ {
		<-c
	}
}
```