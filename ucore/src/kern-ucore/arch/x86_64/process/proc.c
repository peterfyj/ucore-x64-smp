#include <proc.h>
#include <pmm.h>
#include <slab.h>
#include <string.h>
#include <types.h>
#include <stdlib.h>

void kernel_thread_entry(void);
void forkret(void);

// alloc_proc - create a proc struct and init fields
struct proc_struct *
alloc_proc(void) {
    struct proc_struct *proc = kmalloc(sizeof(struct proc_struct));
    if (proc != NULL) {
        proc->state = PROC_UNINIT;
        proc->pid = -1;
        proc->runs = 0;
        proc->kstack = 0;
        proc->need_resched = 0;
        proc->parent = NULL;
        proc->mm = NULL;
        memset(&(proc->context), 0, sizeof(struct context));
        proc->tf = NULL;
        proc->cr3 = PADDR(init_pgdir_get());
        proc->flags = 0;
        memset(proc->name, 0, PROC_NAME_LEN);
        proc->wait_state = 0;
        proc->cptr = proc->optr = proc->yptr = NULL;
        list_init(&(proc->thread_group));
        proc->rq = NULL;
        list_init(&(proc->run_link));
        proc->time_slice = 0;
        proc->sem_queue = NULL;
        event_box_init(&(proc->event_box));
        proc->fs_struct = NULL;
    }
    return proc;
}

int
copy_thread(uint32_t clone_flags, struct proc_struct *proc, uintptr_t rsp, struct trapframe *tf) {
    uintptr_t kstacktop = proc->kstack + KSTACKSIZE;
    proc->tf = (struct trapframe *)kstacktop - 1;
    *(proc->tf) = *tf;
    proc->tf->tf_regs.reg_rax = 0;
    proc->tf->tf_rsp = (rsp != 0) ? rsp : kstacktop;
    proc->tf->tf_rflags |= FL_IF;

    proc->context.rip = (uintptr_t)forkret;
    proc->context.rsp = (uintptr_t)(proc->tf);

	return 0;
}


// kernel_thread - create a kernel thread using "fn" function
// NOTE: the contents of temp trapframe tf will be copied to 
//       proc->tf in do_fork-->copy_thread function
int
kernel_thread(int (*fn)(void *), void *arg, uint32_t clone_flags) {
    struct trapframe tf;
    memset(&tf, 0, sizeof(struct trapframe));
    tf.tf_cs = KERNEL_CS;
    tf.tf_ds = tf.tf_es = tf.tf_ss = KERNEL_DS;
    tf.tf_regs.reg_rdi = (uint64_t)fn;
    tf.tf_regs.reg_rsi = (uint64_t)arg;
    tf.tf_rip = (uint64_t)kernel_thread_entry;
    return do_fork(clone_flags | CLONE_VM, 0, &tf);
}

int
kernel_execve(const char *name, const char **argv) {
    int argc = 0, ret;
    while (argv[argc] != NULL) {
        argc ++;
    }
    asm volatile (
        "int %1;"
        : "=a" (ret)
        : "i" (T_SYSCALL), "0" (SYS_exec), "D" (name), "S" (argc), "d" (argv)
        : "memory");
    return ret;
}
