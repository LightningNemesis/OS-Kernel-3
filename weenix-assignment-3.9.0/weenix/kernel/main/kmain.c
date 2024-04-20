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

extern void *sunghan_test(int, void *);
extern void *sunghan_deadlock_test(int, void *);
extern void *faber_thread_test(int, void *);
#ifdef __VFS__
extern void *vfstest_main(int, void *);
extern int faber_fs_thread_test(kshell_t *ksh, int argc, char **argv);
extern int faber_directory_test(kshell_t *ksh, int argc, char **argv);
#endif
int my_faber_thread_test();
int my_sunghan_test();
int my_sunghan_deadlock_test();
#ifdef __VFS__
int my_vfs_test();
#endif

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
        // NOT_YET_IMPLEMENTED("VFS: idleproc_run");

        curproc->p_cwd = vfs_root_vn;
        initthr->kt_proc->p_cwd = vfs_root_vn;
        vref(curproc->p_cwd);
        vref(initthr->kt_proc->p_cwd);
        // Instance of byte device
        // Register Byte device
        // Call make node with three different device ID's

        /* Here you need to make the null, zero, and tty devices using mknod */
        /* You can't do this until you have VFS, check the include/drivers/dev.h
         * file for macros with the device ID's you will need to pass to mknod */
        // NOT_YET_IMPLEMENTED("VFS: idleproc_run");
        do_mkdir("/dev");
        // do_mkdir("/dev/null");
        // do_mkdir("/dev/zero");
        // do_mkdir("/dev/tty0");
        do_mknod("/dev/null", S_IFCHR, MKDEVID(1, 0));
        do_mknod("/dev/zero", S_IFCHR, MKDEVID(1, 1));
        do_mknod("/dev/tty0", S_IFCHR, MKDEVID(2, 0));
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

#ifdef __MTP__
        kthread_reapd_shutdown();
#endif

#ifdef __SHADOWD__
        /* wait for shadowd to shutdown */
        shadowd_shutdown();
#endif

#ifdef __VFS__
        /* Shutdown the vfs: */
        dbg_print("weenix: vfs shutdown...\n");
        vput(curproc->p_cwd);
        if (vfs_shutdown())
                panic("vfs shutdown FAILED!!\n");

#endif

                /* Shutdown the pframe system */
#ifdef __S5FS__
        pframe_shutdown();
#endif

        dbg_print("\nweenix: halted cleanly!\n");
        GDB_CALL_HOOK(shutdown);
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
        // NOT_YET_IMPLEMENTED("PROCS: initproc_run");

#ifndef __DRIVERS__
        dbg(DBG_PRINT, "(GRADING1A)\n");
        faber_thread_test(0, NULL);
#endif

#ifdef __DRIVERS__

        kshell_add_command("sunghan", my_sunghan_test, "Run sunghan_test().");
        kshell_add_command("deadlock", my_sunghan_deadlock_test, "Run sunghan_deadlock_test().");
        kshell_add_command("faber", my_faber_thread_test, "Run faber_thread_test().");
#ifdef __VFS__
        kshell_add_command("vfstest", my_vfs_test, "Run vfstest().");
        kshell_add_command("thrtest", faber_fs_thread_test, "Run vfstest().");
        kshell_add_command("dirtest", faber_directory_test, "Run vfstest().");
        dbg(DBG_PRINT, "(GRADING2B)\n");
#endif
#ifdef __VM__

        // run init.c
        char *const argv[] = {NULL};
        char *const envp[] = {NULL};
        kernel_execve("/sbin/init", argv, envp);

#else

        kshell_t *kshell = kshell_create(0);
        while (kshell_execute_next(kshell))
        {
        }
        kshell_destroy(kshell);

#endif /* __VM__ */
#endif /* __DRIVERS__ */

        return NULL;
}

#ifdef __VFS__
int my_vfs_test()
{
        proc_t *pt_vfs;
        kthread_t *kt_vfs;
        int status;
        pt_vfs = proc_create("VFS");

        kt_vfs = kthread_create(pt_vfs, vfstest_main, 1, NULL);

        sched_make_runnable(kt_vfs);
        do_waitpid(pt_vfs->p_pid, 0, &status);
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return 0;
}
#endif

int my_sunghan_test()
{
        proc_t *pt_sunghan;
        kthread_t *kt_sunghan;
        int status;

        // Create the sunghan process
        pt_sunghan = proc_create("SUNGHAN");

        kt_sunghan = kthread_create(pt_sunghan, sunghan_test, 0, NULL);

        sched_make_runnable(kt_sunghan);
        do_waitpid(pt_sunghan->p_pid, 0, &status);

        dbg(DBG_PRINT, "(GRADING1D 1)\n");

        return 0;
}

int my_sunghan_deadlock_test()
{
        proc_t *pt_sunghan_deadlock;
        kthread_t *kt_sunghan_deadlock;
        int status;

        // Create the sunghan deadlock process
        pt_sunghan_deadlock = proc_create("SUNGHANDEADLOCK");

        kt_sunghan_deadlock = kthread_create(pt_sunghan_deadlock, sunghan_deadlock_test, 0, NULL);

        sched_make_runnable(kt_sunghan_deadlock);
        do_waitpid(pt_sunghan_deadlock->p_pid, 0, &status);

        dbg(DBG_PRINT, "(GRADING1D 2)\n");

        return 0;
}

int my_faber_thread_test()
{
        proc_t *pt_faber;
        kthread_t *kt_faber;
        int status;

        // Create the faber process
        pt_faber = proc_create("FABER");

        kt_faber = kthread_create(pt_faber, faber_thread_test, 0, NULL);

        sched_make_runnable(kt_faber);
        do_waitpid(pt_faber->p_pid, 0, &status);

        dbg(DBG_PRINT, "(GRADING1C)\n");

        return 0;
}

