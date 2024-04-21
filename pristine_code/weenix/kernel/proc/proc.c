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

#include "kernel.h"
#include "config.h"
#include "globals.h"
#include "errno.h"

#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"
#include "util/printf.h"

#include "proc/kthread.h"
#include "proc/proc.h"
#include "proc/sched.h"
#include "proc/proc.h"

#include "mm/slab.h"
#include "mm/page.h"
#include "mm/mmobj.h"
#include "mm/mm.h"
#include "mm/mman.h"

#include "vm/vmmap.h"

#include "fs/vfs.h"
#include "fs/vfs_syscall.h"
#include "fs/vnode.h"
#include "fs/file.h"

proc_t *curproc = NULL; /* global */
static slab_allocator_t *proc_allocator = NULL;

static list_t _proc_list;
static proc_t *proc_initproc = NULL; /* Pointer to the init process (PID 1) */

void proc_init()
{
        list_init(&_proc_list);
        proc_allocator = slab_allocator_create("proc", sizeof(proc_t));
        KASSERT(proc_allocator != NULL);
}

proc_t *
proc_lookup(int pid)
{
        proc_t *p;
        list_iterate_begin(&_proc_list, p, proc_t, p_list_link)
        {
                if (p->p_pid == pid)
                {
                        return p;
                }
        }
        list_iterate_end();
        return NULL;
}

list_t *
proc_list()
{
        return &_proc_list;
}

size_t
proc_info(const void *arg, char *buf, size_t osize)
{
        const proc_t *p = (proc_t *)arg;
        size_t size = osize;
        proc_t *child;

        KASSERT(NULL != p);
        KASSERT(NULL != buf);

        iprintf(&buf, &size, "pid:          %i\n", p->p_pid);
        iprintf(&buf, &size, "name:         %s\n", p->p_comm);
        if (NULL != p->p_pproc)
        {
                iprintf(&buf, &size, "parent:       %i (%s)\n",
                        p->p_pproc->p_pid, p->p_pproc->p_comm);
        }
        else
        {
                iprintf(&buf, &size, "parent:       -\n");
        }

#ifdef __MTP__
        int count = 0;
        kthread_t *kthr;
        list_iterate_begin(&p->p_threads, kthr, kthread_t, kt_plink)
        {
                ++count;
        }
        list_iterate_end();
        iprintf(&buf, &size, "thread count: %i\n", count);
#endif

        if (list_empty(&p->p_children))
        {
                iprintf(&buf, &size, "children:     -\n");
        }
        else
        {
                iprintf(&buf, &size, "children:\n");
        }
        list_iterate_begin(&p->p_children, child, proc_t, p_child_link)
        {
                iprintf(&buf, &size, "     %i (%s)\n", child->p_pid, child->p_comm);
        }
        list_iterate_end();

        iprintf(&buf, &size, "status:       %i\n", p->p_status);
        iprintf(&buf, &size, "state:        %i\n", p->p_state);

#ifdef __VFS__
#ifdef __GETCWD__
        if (NULL != p->p_cwd)
        {
                char cwd[256];
                lookup_dirpath(p->p_cwd, cwd, sizeof(cwd));
                iprintf(&buf, &size, "cwd:          %-s\n", cwd);
        }
        else
        {
                iprintf(&buf, &size, "cwd:          -\n");
        }
#endif /* __GETCWD__ */
#endif

#ifdef __VM__
        iprintf(&buf, &size, "start brk:    0x%p\n", p->p_start_brk);
        iprintf(&buf, &size, "brk:          0x%p\n", p->p_brk);
#endif

        return size;
}

size_t
proc_list_info(const void *arg, char *buf, size_t osize)
{
        size_t size = osize;
        proc_t *p;

        KASSERT(NULL == arg);
        KASSERT(NULL != buf);

#if defined(__VFS__) && defined(__GETCWD__)
        iprintf(&buf, &size, "%5s %-13s %-18s %-s\n", "PID", "NAME", "PARENT", "CWD");
#else
        iprintf(&buf, &size, "%5s %-13s %-s\n", "PID", "NAME", "PARENT");
#endif

        list_iterate_begin(&_proc_list, p, proc_t, p_list_link)
        {
                char parent[64];
                if (NULL != p->p_pproc)
                {
                        snprintf(parent, sizeof(parent),
                                 "%3i (%s)", p->p_pproc->p_pid, p->p_pproc->p_comm);
                }
                else
                {
                        snprintf(parent, sizeof(parent), "  -");
                }

#if defined(__VFS__) && defined(__GETCWD__)
                if (NULL != p->p_cwd)
                {
                        char cwd[256];
                        lookup_dirpath(p->p_cwd, cwd, sizeof(cwd));
                        iprintf(&buf, &size, " %3i  %-13s %-18s %-s\n",
                                p->p_pid, p->p_comm, parent, cwd);
                }
                else
                {
                        iprintf(&buf, &size, " %3i  %-13s %-18s -\n",
                                p->p_pid, p->p_comm, parent);
                }
#else
                iprintf(&buf, &size, " %3i  %-13s %-s\n",
                        p->p_pid, p->p_comm, parent);
#endif
        }
        list_iterate_end();
        return size;
}

static pid_t next_pid = 0;

/**
 * Returns the next available PID.
 *
 * Note: Where n is the number of running processes, this algorithm is
 * worst case O(n^2). As long as PIDs never wrap around it is O(n).
 *
 * @return the next available PID
 */
static int
_proc_getid()
{
        proc_t *p;
        pid_t pid = next_pid;
        while (1)
        {
        failed:
                list_iterate_begin(&_proc_list, p, proc_t, p_list_link)
                {
                        if (p->p_pid == pid)
                        {
                                if ((pid = (pid + 1) % PROC_MAX_COUNT) == next_pid)
                                {
                                        return -1;
                                }
                                else
                                {
                                        goto failed;
                                }
                        }
                }
                list_iterate_end();
                next_pid = (pid + 1) % PROC_MAX_COUNT;
                return pid;
        }
}

/*
 * The new process, although it isn't really running since it has no
 * threads, should be in the PROC_RUNNING state.
 *
 * Don't forget to set proc_initproc when you create the init
 * process. You will need to be able to reference the init process
 * when reparenting processes to the init process.
 */
proc_t *
proc_create(char *name)
{
        /* Allocate space for proc <name> */
        proc_t *proc = (proc_t *)slab_obj_alloc(proc_allocator);

        /* Initialize proc members */
        proc->p_pid = _proc_getid();                // Get the next available PID
        strncpy(proc->p_comm, name, PROC_NAME_LEN); // Copy the name to the process's name
        list_init(&proc->p_threads);                // Initialize the process's thread list
        list_init(&proc->p_children);               // Initialize the process's children list
        proc->p_status = 0;                         // Set the process's exit status
        proc->p_state = PROC_RUNNING;               // Set the process's state (DEFAULT)
        sched_queue_init(&proc->p_wait);            // Initialize the process's wait queue
        proc->p_pagedir = pt_create_pagedir();      // Create a new page directory for the process
        list_link_init(&proc->p_list_link);         // Initialize the process's list link
        list_link_init(&proc->p_child_link);        // Initialize the process's child link

        KASSERT(PID_IDLE != proc->p_pid || list_empty(&_proc_list));    /* pid can only be PID_IDLE if this is the first process */
        KASSERT(PID_INIT != proc->p_pid || PID_IDLE == curproc->p_pid); /* pid can only be PID_INIT if the running process is the "idle" process */
        dbg(DBG_PRINT, "(GRADING1A 2.a)\n");

        list_insert_tail(&_proc_list, &proc->p_list_link); // appending to Global process list

#ifdef __VFS__
        // Initialize the process's current working directory to the root of the virtual filesystem, if available
        proc->p_cwd = vfs_root_vn; // Initial assignment, works even if vfs_root_vn is NULL
        if (vfs_root_vn)
        {
                vref(vfs_root_vn); // Increment the reference count only if vfs_root_vn is not NULL
        }

        // Initialize all file descriptors to NULL
        for (int i = 0; i < NFILES; ++i)
        {
                proc->p_files[i] = NULL;
        }
#endif /* __VFS__ */

        if (proc->p_pid == PID_INIT)
        {
                proc_initproc = proc;
                dbg(DBG_PRINT, "(GRADING1A)\n");
        }
        dbg(DBG_PRINT, "(GRADING1A)\n");

        if (proc->p_pid == PID_IDLE)
        {
                proc->p_pproc = NULL;
                dbg(DBG_PRINT, "(GRADING1A)\n");
        }
        else
        {
                proc->p_pproc = curproc;
                list_insert_tail(&curproc->p_children, &proc->p_child_link); // appending to Parent process's children list
                dbg(DBG_PRINT, "(GRADING1A)\n");
        }

        dbg(DBG_PRINT, "(GRADING1A)\n");

        return proc;
}

/**
 * Cleans up as much as the process as can be done from within the
 * process. This involves:
 *    - Closing all open files (VFS)
 *    - Cleaning up VM mappings (VM)
 *    - Waking up its parent if it is waiting
 *    - Reparenting any children to the init process
 *    - Setting its status and state appropriately
 *
 * The parent will finish destroying the process within do_waitpid (make
 * sure you understand why it cannot be done here). Until the parent
 * finishes destroying it, the process is informally called a 'zombie'
 * process.
 *
 * This is also where any children of the current process should be
 * reparented to the init process (unless, of course, the current
 * process is the init process. However, the init process should not
 * have any children at the time it exits).
 *
 * Note: You do _NOT_ have to special case the idle process. It should
 * never exit this way.
 *
 * @param status the status to exit the process with
 */
void proc_cleanup(int status)
{
        dbg(DBG_TEMP, "Inside proc_cleanup()");

        KASSERT(NULL != proc_initproc);    /* "init" process must exist and proc_initproc initialized */
        KASSERT(1 <= curproc->p_pid);      /* this process must not be "idle" process */
        KASSERT(NULL != curproc->p_pproc); /* this process must have a parent when this function is entered */
        dbg(DBG_PRINT, "(GRADING1A 2.b)\n");

        curproc->p_status = status;   // setting curproc status to the given status
        curproc->p_state = PROC_DEAD; // setting curproc state to DEAD

        if (curproc != proc_initproc)
        {
                dbg(DBG_TEMP, "Reparenting all children processes of curproc to init_proc");
                // Reparenting all children processes of curproc to init_proc

                dbg(DBG_TEMP, "Iterating through curproc's children list, reparenting to init_proc\n");
                proc_t *p;
                list_iterate_begin(&curproc->p_children, p, proc_t, p_child_link)
                {
                        dbg(DBG_TEMP, "appending to init process's children list\n");
                        list_insert_tail(&proc_initproc->p_children, &p->p_child_link); // appending to init process's children list

                        dbg(DBG_TEMP, "setting the parent process to init process\n");
                        p->p_pproc = proc_initproc; // setting the parent process to init process
                }
                list_iterate_end();
                dbg(DBG_PRINT, "(GRADING1A)\n");
        }
        dbg(DBG_PRINT, "(GRADING1A)\n");

#ifdef __VFS__
        // Release the current working directory if it is set
        if (curproc->p_cwd)
        {
                vput(curproc->p_cwd);
                curproc->p_cwd = NULL; // Ensure the pointer is cleared after release
        }

        // Iterate over the file descriptors array and release any open files
        for (int i = 0; i < NFILES; ++i)
        {
                if (curproc->p_files[i])
                {
                        fput(curproc->p_files[i]);  // Release the file
                        curproc->p_files[i] = NULL; // Clear the file descriptor slot
                }
        }
#endif

        // Waking up its parent if it is waiting
        if (curproc->p_pproc->p_wait.tq_size > 0)
        {
                dbg(DBG_TEMP, "Waking up its parent if it is waiting\n");
                sched_wakeup_on(&curproc->p_pproc->p_wait);
                dbg(DBG_PRINT, "(GRADING1A)\n");
        }
        dbg(DBG_PRINT, "(GRADING1A)\n");

        KASSERT(NULL != curproc->p_pproc);      /* this process must still have a parent when this function returns */
        KASSERT(KT_EXITED == curthr->kt_state); /* the thread in this process should be in the KT_EXITED state when this function returns */
        dbg(DBG_PRINT, "(GRADING1A 2.b)\n");

        dbg(DBG_PRINT, "(GRADING1A)\n");
}

/*
 * This has nothing to do with signals and kill(1).
 *
 * Calling this on the current process is equivalent to calling
 * do_exit().
 *
 * In Weenix, this is only called from proc_kill_all.
 */
void proc_kill(proc_t *p, int status)
{
        if (p == curproc)
        {
                do_exit(status);
                dbg(DBG_PRINT, "(GRADING1A)\n");
        }
        dbg(DBG_PRINT, "(GRADING1A)\n");

        kthread_t *thr;
        list_iterate_begin(&p->p_threads, thr, kthread_t, kt_plink)
        {
                thr->kt_cancelled = 1;
                kthread_cancel(thr, (void *)status);
        }
        list_iterate_end();
        dbg(DBG_PRINT, "(GRADING1A)\n");
}

/*
 * Remember, proc_kill on the current process will _NOT_ return.
 * Don't kill direct children of the idle process.
 *
 * In Weenix, this is only called by sys_halt.
 */
void proc_kill_all()
{
        proc_t *proc;
        list_iterate_begin(&_proc_list, proc, proc_t, p_list_link)
        {
                // Don't kill the idle process or the current process in this loop
                if (proc->p_pid != PID_IDLE && proc != curproc && proc->p_pproc->p_pid != PID_IDLE)
                {
                        proc_kill(proc, 0); // Kill the process
                        dbg(DBG_PRINT, "(GRADING1A)\n");
                }
        }
        list_iterate_end();

        // Now consider killing the current process if it's not a direct child of the idle process
        if (curproc->p_pid != PID_IDLE && curproc->p_pproc->p_pid != PID_IDLE)
        {
                proc_kill(curproc, 0); // Kill the current process
                dbg(DBG_PRINT, "(GRADING1A)\n");
        }

        dbg(DBG_PRINT, "(GRADING1A)\n");
}

/*
 * This function is only called from kthread_exit.
 *
 * Unless you are implementing MTP, this just means that the process
 * needs to be cleaned up and a new thread needs to be scheduled to
 * run. If you are implementing MTP, a single thread exiting does not
 * necessarily mean that the process should be exited.
 */
void proc_thread_exited(void *retval)
{
        // clean up the process
        dbg(DBG_TEMP, "Calling proc_cleanup()\n");
        proc_cleanup((int)retval);
        dbg(DBG_PRINT, "(GRADING1A)\n");
}

/* If pid is -1 dispose of one of the exited children of the current
 * process and return its exit status in the status argument, or if
 * all children of this process are still running, then this function
 * blocks on its own p_wait queue until one exits.
 *
 * If pid is greater than 0 and the given pid is a child of the
 * current process then wait for the given pid to exit and dispose
 * of it.
 *
 * If the current process has no children, or the given pid is not
 * a child of the current process return -ECHILD.
 *
 * Pids other than -1 and positive numbers are not supported.
 * Options other than 0 are not supported.
 */

pid_t do_waitpid(pid_t pid, int options, int *status)
{
        proc_t *child;
        list_link_t *link;

        while (1)
        {                      // Loop to handle waking up after sleep
                int found = 0; // Flag to indicate if a matching child process has been found

                // Iterate through the list of child processes
                list_iterate_begin(&curproc->p_children, child, proc_t, p_child_link)
                {
                        if (pid == -1 || child->p_pid == pid)
                        {                  // Check if the child matches the criteria
                                found = 1; // Matching child process found
                                if (child->p_state == PROC_DEAD)
                                { // Child process has exited
                                        if (status != NULL)
                                        {
                                                *status = child->p_status; // Set the exit status
                                        }

                                        // Cleanup: Remove child from process lists and destroy resources
                                        list_remove(&child->p_child_link);
                                        list_remove(&child->p_list_link);
                                        pt_destroy_pagedir(child->p_pagedir); // Destroy the page directory
                                        slab_obj_free(proc_allocator, child); // Free the process object

                                        // Return the PID of the found and cleaned-up child process
                                        return child->p_pid;
                                }
                        }
                }
                list_iterate_end();

                if (!found)
                {
                        return -ECHILD; // No matching child found
                }

                // If we reach here, the child process has not exited yet, so sleep
                sched_sleep_on(&curproc->p_wait);
        }

        // Unreachable code, as the loop handles all logic paths
}

/*
 * Cancel all threads and join with them (if supporting MTP), and exit from the current
 * thread.
 *
 * @param status the exit status of the process
 */

void do_exit(int status)
{
        dbg(DBG_TEMP, "Entering do_exit()\n");

        kthread_t *thr;
        list_iterate_begin(&curproc->p_threads, thr, kthread_t, kt_plink)
        {
                dbg(DBG_TEMP, "Iterating through the curproc's threads, setting thr's kt_cancelled = 1\n");
                thr->kt_cancelled = 1;
        }
        list_iterate_end();

        dbg(DBG_TEMP, "Calling kthread_exit()\n");
        kthread_exit((void *)status);
        dbg(DBG_PRINT, "(GRADING1A)\n");
}
