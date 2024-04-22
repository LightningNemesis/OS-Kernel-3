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

#include "util/string.h"
#include "util/debug.h"

#include "mm/mmobj.h"
#include "mm/pframe.h"
#include "mm/mm.h"
#include "mm/page.h"
#include "mm/slab.h"
#include "mm/tlb.h"

int anon_count = 0; /* for debugging/verification purposes */

static slab_allocator_t *anon_allocator;

static void anon_ref(mmobj_t *o);
static void anon_put(mmobj_t *o);
static int anon_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf);
static int anon_fillpage(mmobj_t *o, pframe_t *pf);
static int anon_dirtypage(mmobj_t *o, pframe_t *pf);
static int anon_cleanpage(mmobj_t *o, pframe_t *pf);

static mmobj_ops_t anon_mmobj_ops = {
    .ref = anon_ref,
    .put = anon_put,
    .lookuppage = anon_lookuppage,
    .fillpage = anon_fillpage,
    .dirtypage = anon_dirtypage,
    .cleanpage = anon_cleanpage};

/*
 * This function is called at boot time to initialize the
 * anonymous page sub system. Currently it only initializes the
 * anon_allocator object.
 */
void anon_init()
{
        // NOT_YET_IMPLEMENTED("VM: anon_init");

        // Initialize the anon_allocator obj
        anon_allocator = slab_allocator_create("anonobj", sizeof(mmobj_t));

        // Check if the anon_allocator obj is NULL
        if (anon_allocator == NULL)
        {
                // Print an error message
                dbg(DBG_ERROR, "ERROR: anon_allocator is NULL\n");
        }

        // Increment the anon_count
        anon_count++;

        // Print a debug message
        dbg(DBG_PRINT, "DEBUG: anon_init() called\n");
}

/*
 * You'll want to use the anon_allocator to allocate the mmobj to
 * return, then initialize it. Take a look in mm/mmobj.h for
 * definitions which can be of use here. Make sure your initial
 * reference count is correct.
 */
mmobj_t *anon_create()
{
        // NOT_YET_IMPLEMENTED("VM: anon_create");
        // return NULL;

        // Use anon_allocator to allocate the mmobj
        mmobj_t *mmobj = (mmobj_t *)slab_obj_alloc(anon_allocator);

        // Check if the mmobj is NULL
        if (mmobj == NULL)
        {
                // Print an error message
                dbg(DBG_ERROR, "ERROR: mmobj is NULL\n");
        }

        // Initialize the mmobj
        mmobj_init(mmobj, &anon_mmobj_ops);

        // Set the reference count to 1
        mmobj->mmo_refcount = 1;

        // Return the mmobj
        return mmobj;
}

/* Implementation of mmobj entry points: */

/*
 * Increment the reference count on the object.
 */
static void
anon_ref(mmobj_t *o)
{
        // NOT_YET_IMPLEMENTED("VM: anon_ref");

        // Increment the reference count
        o->mmo_refcount++;
}

/*
 * Decrement the reference count on the object. If, however, the
 * reference count on the object reaches the number of resident
 * pages of the object, we can conclude that the object is no
 * longer in use and, since it is an anonymous object, it will
 * never be used again. You should unpin and uncache all of the
 * object's pages and then free the object itself.
 */
static void
anon_put(mmobj_t *o)
{
        // NOT_YET_IMPLEMENTED("VM: anon_put");

        // Decrement the reference count
        o->mmo_refcount--;

        // Check if the reference count is equal to the number of resident pages
        if (o->mmo_refcount == o->mmo_nrespages)
        {
                // Unpin and uncache all of the object's pages
                pframe_t *pf;
                list_iterate_begin(&o->mmo_respages, pf, pframe_t, pf_olink)
                {
                        pframe_unpin(pf);
                        pframe_free(pf);
                }
                list_iterate_end();

                // Free the object
                slab_obj_free(anon_allocator, o);
        }
}

/* Get the corresponding page from the mmobj. No special handling is
 * required. */
static int
anon_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf)
{
        // NOT_YET_IMPLEMENTED("VM: anon_lookuppage");
        // return -1;

        // create page frame
        pframe_t *page_frame;

        // iterate over the resident pages
        list_iterate_begin(&o->mmo_respages, page_frame, pframe_t, pf_olink)
        {
                // check if the page frame is the one we are looking for
                if (page_frame->pf_pagenum == pagenum)
                {
                        // pin the page frame
                        pframe_pin(page_frame);
                        // set the page frame
                        *pf = page_frame;
                        // return 0
                        return 0;
                }
        }
        list_iterate_end();

        // create a new page frame
        int ret = pframe_get(o, pagenum, &page_frame);

        // check if the return value is less than 0
        if (ret < 0)
        {
                // return -1
                return -1;
        }

        // pin the page frame
        pframe_pin(page_frame);

        // add the page frame to the resident pages list
        list_insert_tail(&o->mmo_respages, &page_frame->pf_olink);

        // set the page frame
        *pf = page_frame;

        // return 0
        return 0;
}

/* The following three functions should not be difficult. */

static int
anon_fillpage(mmobj_t *o, pframe_t *pf)
{
        // NOT_YET_IMPLEMENTED("VM: anon_fillpage");
        // return 0;

        // check if page frame is valid
        KASSERT(pf != NULL);
        KASSERT(o != NULL);

        // fill the page frame with zeros
        memset(pf->pf_addr, 0, PAGE_SIZE);

        // return 0
        return 0;
}

static int
anon_dirtypage(mmobj_t *o, pframe_t *pf)
{
        // NOT_YET_IMPLEMENTED("VM: anon_dirtypage");
        // return -1;

        // check if page frame is valid
        KASSERT(pf != NULL);
        KASSERT(o != NULL);

        // mark the page as dirty
        pf->pf_flags |= PF_DIRTY;

        // return 0
        return 0;
}

static int
anon_cleanpage(mmobj_t *o, pframe_t *pf)
{
        // NOT_YET_IMPLEMENTED("VM: anon_cleanpage");
        // return -1;

        // check if page frame is valid
        KASSERT(pf != NULL);
        KASSERT(o != NULL);

        // mark the page as clean
        pf->pf_flags &= ~PF_DIRTY;

        // return 0
        return 0;
}
