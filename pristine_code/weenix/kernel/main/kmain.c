/******************************************************************************/
/* Important Spring 2024 CSCI 402 usage information:                          */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/*         53616c7465645f5fd1e93dbf35cbffa3aef28f8c01d8cf2ffc51ef62b26a       */
/*         f9bda5a68e5ed8c972b17bab0f42e24b19daa7bd408305b1f7bd6c7208c1       */
/*         0e36230e913039b3046dd5fd0ba706a624d33dbaa4d6aab02c82fe09f561       */
/*         01b0fd977b0051f0b0ce0c69f7db857b1b5e007be2db6d42894bf93de848       */
/*         806d9152bd5715e9                                                   */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove or alter this comment block.          */
/* If this comment block is removed or altered in a submitted file, 20 points */
/*         will be deducted.                                                  */
/******************************************************************************/

#include "types.h"
#include "globals.h"
#include "kernel.h"
#include "errno.h"

#include "util/gdb.h"
#include "util/init.h"
#include "util/debug.h"
#include "util/string.h"
#include "util/printf.h"

#include "mm/mm.h"
#include "mm/page.h"
#include "mm/pagetable.h"
#include "mm/pframe.h"

#include "vm/vmmap.h"
#include "vm/shadowd.h"
#include "vm/shadow.h"
#include "vm/anon.h"

#include "main/acpi.h"
#include "main/apic.h"
#include "main/interrupt.h"
#include "main/gdt.h"

#include "proc/sched.h"
#include "proc/proc.h"
#include "proc/kthread.h"

#include "drivers/dev.h"
#include "drivers/blockdev.h"
#include "drivers/disk/ata.h"
#include "drivers/tty/virtterm.h"
#include "drivers/pci.h"

#include "api/exec.h"
#include "api/syscall.h"

#include "fs/vfs.h"
#include "fs/vnode.h"
#include "fs/vfs_syscall.h"
#include "fs/fcntl.h"
#include "fs/stat.h"

#include "test/kshell/kshell.h"
#include "test/s5fs_test.h"

GDB_DEFINE_HOOK(initialized)

void *bootstrap(int arg1, void *arg2);
void *idleproc_run(int arg1, void *arg2);
kthread_t *initproc_create(void);
void *initproc_run(int arg1, void *arg2);
void *final_shutdown(void);

extern int faber_fs_thread_test(kshell_t *ksh, int argc, char **argv);
extern int faber_directory_test(kshell_t *ksh, int argc, char **argv);

/**
 * This function is called from kmain, however it is not running in a
 * thread context yet. It should create the idle process which will
 * start executing idleproc_run() in a real thread context.  To start
 * executing in the new process's context call context_make_active(),
 * passing in the appropriate context. This function should _NOT_
 * return.
 *
 * Note: Don't forget to set curproc and curthr appropriately.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
void *
bootstrap(int arg1, void *arg2)
{
        /* If the next line is removed/altered in your submission, 20 points will be deducted. */
        dbgq(DBG_TEST, "SIGNATURE: 53616c7465645f5f2af2e2b4cd456a3e67146f447bcf037a0ca35081c931d7d7b285d622d1b49f20cb0e0ac91e4774d4\n");
        /* necessary to finalize page table information */
        pt_template_init();

        /* Initializing the process, thread, sched management system */
        proc_init();
        kthread_init();

        /* create the idle process */
        proc_t *idle_proc = proc_create("idle");
        KASSERT(NULL != idle_proc);
        KASSERT(PID_IDLE == idle_proc->p_pid);
        dbg(DBG_PRINT, "(GRADING1A 1.a)\n");

        /* create the idle thread */
        kthread_t *idle_thread = kthread_create(idle_proc, idleproc_run, 0, NULL);
        KASSERT(idle_thread != NULL);
        dbg(DBG_PRINT, "(GRADING1A 1.a)\n");

        /* set the current process and thread */
        curproc = idle_proc;
        curthr = idle_thread;
        KASSERT(NULL != curthr);
        dbg(DBG_PRINT, "(GRADING1A 1.a)\n");
        dbg(DBG_PRINT, "(GRADING1A)\n");

        context_make_active(&curthr->kt_ctx);

        dbg(DBG_PRINT, "(GRADING1A)\n");
        return NULL;
}

/**
 * Once we're inside of idleproc_run(), we are executing in the context of the
 * first process-- a real context, so we can finally begin running
 * meaningful code.
 *
 * This is the body of process 0. It should initialize all that we didn't
 * already initialize in kmain(), launch the init process (initproc_run),
 * wait for the init process to exit, then halt the machine.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
void *
idleproc_run(int arg1, void *arg2)
{
        int status;
        pid_t child;

        /* create init proc */
        kthread_t *initthr = initproc_create();
        init_call_all();
        GDB_CALL_HOOK(initialized);

        /* Create other kernel threads (in order) */

#ifdef __VFS__
        /* Once you have VFS remember to set the current working directory
         * of the idle and init processes */
        curproc->p_cwd = vfs_root_vn;
        vref(vfs_root_vn);
        initthr->kt_proc->p_cwd = vfs_root_vn;
        vref(initthr->kt_proc->p_cwd);

        /* Here you need to make the null, zero, and tty devices using mknod */
        /* You can't do this until you have VFS, check the include/drivers/dev.h
         * file for macros with the device ID's you will need to pass to mknod */
        do_mkdir("/dev");
        do_mknod("/dev/null", S_IFCHR, MEM_NULL_DEVID);
        do_mknod("/dev/zero", S_IFCHR, MEM_ZERO_DEVID);
        do_mknod("/dev/tty0", S_IFCHR, MKDEVID(2, 0));
        do_mknod("/dev/tty1", S_IFCHR, MKDEVID(2, 1));
        dbg(DBG_PRINT, "(GRADING2B)\n");

#endif

        /* Finally, enable interrupts (we want to make sure interrupts
         * are enabled AFTER all drivers are initialized) */
        intr_enable();

        /* Run initproc */
        sched_make_runnable(initthr);
        /* Now wait for it */
        child = do_waitpid(-1, 0, &status);
        KASSERT(PID_INIT == child);

        return final_shutdown();
}

/**
 * This function, called by the idle process (within 'idleproc_run'), creates the
 * process commonly refered to as the "init" process, which should have PID 1.
 *
 * The init process should contain a thread which begins execution in
 * initproc_run().
 *
 * @return a pointer to a newly created thread which will execute
 * initproc_run when it begins executing
 */
kthread_t *
initproc_create(void)
{
        dbg(DBG_TEMP, "Entering initproc_create()\n");

        proc_t *init_proc = proc_create("init");
        KASSERT(NULL != init_proc);
        KASSERT(PID_INIT == init_proc->p_pid);
        dbg(DBG_PRINT, "(GRADING1A 1.b)\n");

        list_insert_tail(&curproc->p_children, &init_proc->p_child_link); // appending to Parent process's children list
        dbg(DBG_TEMP, "Appended INIT proc to children list of IDLE proc\n");

        dbg(DBG_TEMP, "Create INIT thread\n");
        kthread_t *init_thread = kthread_create(init_proc, initproc_run, 0, NULL);
        KASSERT(NULL != init_thread);
        dbg(DBG_PRINT, "(GRADING1A 1.b)\n");

        dbg(DBG_TEMP, "Returning from initproc_run()\n");

        dbg(DBG_PRINT, "(GRADING1A)\n");
        return init_thread;
}

/*
 * faber_thread_test() function called as a kshell command
 * sunghan_test() function called as a kshell command
 * sunghan_deadlock_test() function called as a kshell command
 */
extern void *faber_thread_test(int, void *);
int my_faber_thread_test()
{
        proc_t *pt_faber;
        kthread_t *kt_faber;
        int status;

        // Create the faber process
        dbg(DBG_TEMP, "Creating FABER process\n");
        pt_faber = proc_create("FABER");

        // Create the faber thread
        dbg(DBG_TEMP, "Creating FABER thread\n");
        kt_faber = kthread_create(pt_faber, faber_thread_test, 0, NULL);

        // Make the faber thread runnable
        dbg(DBG_TEMP, "Making FABER thread runnable\n");
        sched_make_runnable(kt_faber);

        do_waitpid(pt_faber->p_pid, 0, &status);

        dbg(DBG_PRINT, "(GRADING1C)\n");

        return 0;
}

extern void *sunghan_test(int, void *);
int my_sunghan_test()
{
        proc_t *pt_sunghan;
        kthread_t *kt_sunghan;
        int status;

        // Create the sunghan process
        dbg(DBG_TEMP, "Creating SUNGHAN process\n");
        pt_sunghan = proc_create("SUNGHAN");

        // Create the sunghan thread
        dbg(DBG_TEMP, "Creating SUNGHAN thread\n");
        kt_sunghan = kthread_create(pt_sunghan, sunghan_test, 0, NULL);

        // Make the sunghan thread runnable
        dbg(DBG_TEMP, "Making SUNGHAN thread runnable\n");
        sched_make_runnable(kt_sunghan);

        do_waitpid(pt_sunghan->p_pid, 0, &status);

        dbg(DBG_PRINT, "(GRADING1D 1)\n");

        return 0;
}

extern void *sunghan_deadlock_test(int, void *);
int my_sunghan_deadlock_test()
{
        dbg(DBG_PRINT, "(GRADING1D 2)\n");

        proc_t *pt_sunghan_deadlock;
        kthread_t *kt_sunghan_deadlock;
        int status;

        // Create the sunghan deadlock process
        pt_sunghan_deadlock = proc_create("SUNGHANDEADLOCK");
        dbg(DBG_TEMP, "Creating SUNGHANDEADLOCK process\n");

        // Create the sunghan deadlock thread
        kt_sunghan_deadlock = kthread_create(pt_sunghan_deadlock, sunghan_deadlock_test, 0, NULL);
        dbg(DBG_TEMP, "Creating SUNGHANDEADLOCK thread\n");

        // Make the sunghan deadlock thread runnable
        dbg(DBG_TEMP, "Making SUNGHANDEADLOCK thread runnable\n");
        sched_make_runnable(kt_sunghan_deadlock);

        do_waitpid(pt_sunghan_deadlock->p_pid, 0, &status);

        dbg(DBG_PRINT, "(GRADING1D 2)\n");
        panic("Should never get here!  Code below is to avoid getting a compiler warning!\n");

        return 0;
}

#ifdef __VFS__

extern void *vfstest_main(int, void *);
int my_vfs_test()
{
        proc_t *pt_vfs;
        kthread_t *kt_vfs;
        int status;

        // Create the vfs process
        pt_vfs = proc_create("VFS");

        // Create the vfs thread
        kt_vfs = kthread_create(pt_vfs, vfstest_main, 1, NULL);

        // Make the vfs thread runnable
        sched_make_runnable(kt_vfs);

        do_waitpid(pt_vfs->p_pid, 0, &status);

        return 0;
}

#endif

/**
 * The init thread's function changes depending on how far along your Weenix is
 * developed. Before VM/FI, you'll probably just want to have this run whatever
 * tests you've written (possibly in a new process). After VM/FI, you'll just
 * exec "/sbin/init".
 *
 * Both arguments are unused.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
void *
initproc_run(int arg1, void *arg2)
{

        char *const argv[] = {NULL};
        char *const envp[] = {NULL};

        // Call kernel_execve
        kernel_execve("/usr/bin/hello", argv, envp);

#ifndef __DRIVERS__
        faber_thread_test(0, NULL);
        dbg(DBG_PRINT, "(GRADING1C)\n");
#endif

#ifdef __DRIVERS__
        kshell_add_command("faber_test", my_faber_thread_test, "Run faber_thread_test().");
        kshell_add_command("sunghan_test", my_sunghan_test, "Run sunghan_test().");
        kshell_add_command("deadlock", my_sunghan_deadlock_test, "Run sunghan_deadlock_test().");

#endif

#ifdef __VFS__
        kshell_add_command("vfs_test", my_vfs_test, "Run vfstest_main().");
        kshell_add_command("fstest", faber_fs_thread_test, "Run faber_fs_thread_test().");
        kshell_add_command("dirtest", faber_directory_test, "Run faber_directory_test().");
#endif
        // kshell_t *kshell = kshell_create(0);

        // if (NULL == kshell)
        //         panic("init: Couldn't create kernel shell\n");

        // while (kshell_execute_next(kshell))
        //         ;
        // kshell_destroy(kshell);
        dbg(DBG_PRINT, "(GRADING1B)\n");
        dbg(DBG_PRINT, "(GRADING1A)\n");
        return NULL;
}
