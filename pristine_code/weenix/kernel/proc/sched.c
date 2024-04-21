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

#include "globals.h"
#include "errno.h"

#include "main/interrupt.h"

#include "proc/sched.h"
#include "proc/kthread.h"

#include "util/init.h"
#include "util/debug.h"

static ktqueue_t kt_runq;

static __attribute__((unused)) void
sched_init(void)
{
        sched_queue_init(&kt_runq);
}
init_func(sched_init);

/*** PRIVATE KTQUEUE MANIPULATION FUNCTIONS ***/
/**
 * Enqueues a thread onto a queue.
 *
 * @param q the queue to enqueue the thread onto
 * @param thr the thread to enqueue onto the queue
 */
void ktqueue_enqueue(ktqueue_t *q, kthread_t *thr)
{
        KASSERT(!thr->kt_wchan);
        list_insert_head(&q->tq_list, &thr->kt_qlink);
        thr->kt_wchan = q;
        q->tq_size++;
}

/**
 * Dequeues a thread from the queue.
 *
 * @param q the queue to dequeue a thread from
 * @return the thread dequeued from the queue
 */
kthread_t *
ktqueue_dequeue(ktqueue_t *q)
{
        kthread_t *thr;
        list_link_t *link;

        if (list_empty(&q->tq_list))
                return NULL;

        link = q->tq_list.l_prev;
        thr = list_item(link, kthread_t, kt_qlink);
        list_remove(link);
        thr->kt_wchan = NULL;

        q->tq_size--;

        return thr;
}

/**
 * Removes a given thread from a queue.
 *
 * @param q the queue to remove the thread from
 * @param thr the thread to remove from the queue
 */
static void
ktqueue_remove(ktqueue_t *q, kthread_t *thr)
{
        KASSERT(thr->kt_qlink.l_next && thr->kt_qlink.l_prev);
        list_remove(&thr->kt_qlink);
        thr->kt_wchan = NULL;
        q->tq_size--;
}

/*** PUBLIC KTQUEUE MANIPULATION FUNCTIONS ***/
void sched_queue_init(ktqueue_t *q)
{
        list_init(&q->tq_list);
        q->tq_size = 0;
}

int sched_queue_empty(ktqueue_t *q)
{
        return list_empty(&q->tq_list);
}

/*
 * Similar to sleep on, but the sleep can be cancelled.
 *
 * Don't forget to check the kt_cancelled flag at the correct times.
 *
 * Use the private queue manipulation functions above.
 */
int sched_cancellable_sleep_on(ktqueue_t *q)
{
        if (curthr->kt_cancelled == 0)
        {
                curthr->kt_state = KT_SLEEP_CANCELLABLE;
                ktqueue_enqueue(q, curthr);
                sched_switch();
                dbg(DBG_PRINT, "(GRADING1A)\n");
        }
        dbg(DBG_PRINT, "(GRADING1A)\n");

        if (curthr->kt_cancelled == 1)
        {
                dbg(DBG_PRINT, "(GRADING1A)\n");
                return -EINTR;
        }
        dbg(DBG_PRINT, "(GRADING1A)\n");
        return 0;
}

/*
 * If the thread's sleep is cancellable, we set the kt_cancelled
 * flag and remove it from the queue. Otherwise, we just set the
 * kt_cancelled flag and leave the thread on the queue.
 *
 * Remember, unless the thread is in the KT_NO_STATE or KT_EXITED
 * state, it should be on some queue. Otherwise, it will never be run
 * again.
 */
void sched_cancel(struct kthread *kthr)
{

        kthr->kt_cancelled = 1;

        // check if the sleeping thread passed, is cancellable
        if (kthr->kt_state == KT_SLEEP_CANCELLABLE)
        {
                // remove the thread from the queue
                ktqueue_remove(kthr->kt_wchan, kthr);
                sched_make_runnable(kthr);
                dbg(DBG_PRINT, "(GRADING1A)\n");
        }
        dbg(DBG_PRINT, "(GRADING1A)\n");
}

/*
 * In this function, you will be modifying the run queue, which can
 * also be modified from an interrupt context. In order for thread
 * contexts and interrupt contexts to play nicely, you need to mask
 * all interrupts before reading or modifying the run queue and
 * re-enable interrupts when you are done. This is analagous to
 * locking a mutex before modifying a data structure shared between
 * threads. Masking interrupts is accomplished by setting the IPL to
 * high.
 *
 * Once you have masked interrupts, you need to remove a thread from
 * the run queue and switch into its context from the currently
 * executing context.
 *
 * If there are no threads on the run queue (assuming you do not have
 * any bugs), then all kernel threads are waiting for an interrupt
 * (for example, when reading from a block device, a kernel thread
 * will wait while the block device seeks). You will need to re-enable
 * interrupts and wait for one to occur in the hopes that a thread
 * gets put on the run queue from the interrupt context.
 *
 * The proper way to do this is with the intr_wait call. See
 * interrupt.h for more details on intr_wait.
 *
 * Note: When waiting for an interrupt, don't forget to modify the
 * IPL. If the IPL of the currently executing thread masks the
 * interrupt you are waiting for, the interrupt will never happen, and
 * your run queue will remain empty. This is very subtle, but
 * _EXTREMELY_ important.
 *
 * Note: Don't forget to set curproc and curthr. When sched_switch
 * returns, a different thread should be executing than the thread
 * which was executing when sched_switch was called.
 *
 * Note: The IPL is process specific.
 */
void sched_switch(void)
{
        dbg(DBG_TEMP, "Entering sched_switch()\n");

        uint8_t old_ipl = intr_getipl(); // Store the current IPL

        dbg(DBG_TEMP, "Set IPL to HIGH\n");
        intr_setipl(IPL_HIGH); // Set the IPL to high

        while (list_empty(&kt_runq.tq_list))
        {
                dbg(DBG_TEMP, "Inside while (kt_runq is empty)\n");
                dbg(DBG_TEMP, "Interrupt disabled\n");
                intr_disable();
                dbg(DBG_TEMP, "SET IPL to LOW\n");
                intr_setipl(IPL_LOW); // Set the IPL to low
                dbg(DBG_TEMP, "Wait for an interrupt\n");
                intr_wait(); // Wait for an interrupt
                dbg(DBG_TEMP, "Set the IPL to high\n");
                intr_setipl(IPL_HIGH); // Set the IPL to high

                dbg(DBG_PRINT, "(GRADING1A)\n");
        }
        dbg(DBG_PRINT, "(GRADING1A)\n");

        dbg(DBG_TEMP, "Remove a thread from the run queue & store it in next_thread\n");
        kthread_t *next_thread = ktqueue_dequeue(&kt_runq); // Remove a thread from the run queue & store it in next_thread

        dbg(DBG_TEMP, "Store the current thread in old_current_thread\n");
        kthread_t *old_current_thread = curthr; // Store the current thread in old_current_thread

        curthr = next_thread;           // Set curthr to next_thread
        curproc = next_thread->kt_proc; // Set curproc to the process that curthr belongs to

        dbg(DBG_TEMP, "Switching context\n");
        context_switch(&old_current_thread->kt_ctx, &next_thread->kt_ctx); // Switch into its context from the currently executing context

        dbg(DBG_TEMP, "Set the IPL to the old IPL\n");
        intr_setipl(old_ipl); // Set the IPL to the old IPL

        dbg(DBG_PRINT, "(GRADING1A)\n");
}

/*
 * Since we are modifying the run queue, we _MUST_ set the IPL to high
 * so that no interrupts happen at an inopportune moment.

 * Remember to restore the original IPL before you return from this
 * function. Otherwise, we will not get any interrupts after returning
 * from this function.
 *
 * Using intr_disable/intr_enable would be equally as effective as
 * modifying the IPL in this case. However, in some cases, we may want
 * more fine grained control, making modifying the IPL more
 * suitable. We modify the IPL here for consistency.
 */
void sched_make_runnable(kthread_t *thr)
{
        KASSERT(&kt_runq != thr->kt_wchan); /* the thr argument must not be a thread that's already in the runq */
        KASSERT(thr != NULL);
        dbg(DBG_PRINT, "(GRADING1A 5.a)\n");

        dbg(DBG_TEMP, "Entering sched_make_runnable()\n");

        uint8_t old_ipl = intr_getipl();
        intr_setipl(IPL_HIGH);

        dbg(DBG_TEMP, "SET IPL to HIGH\n");

        // Set the state of the thread to KT_RUN
        thr->kt_state = KT_RUN;

        if (kt_runq.tq_list.l_next == NULL && kt_runq.tq_list.l_prev == NULL)
        {
                dbg(DBG_TEMP, "Initializing kt_runq, before appending to it for the first time\n");
                sched_queue_init(&kt_runq);
                dbg(DBG_PRINT, "(GRADING1A)\n");
        }
        dbg(DBG_PRINT, "(GRADING1A)\n");

        dbg(DBG_TEMP, "Enqueing to kt_runq\n");
        ktqueue_enqueue(&kt_runq, thr); // Enqueue the thread onto the run queue

        dbg(DBG_TEMP, "Restoring IPL\n");
        intr_setipl(old_ipl);

        dbg(DBG_PRINT, "(GRADING1A)\n");
}
