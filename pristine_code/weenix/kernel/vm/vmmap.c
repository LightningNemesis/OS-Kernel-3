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
#include "errno.h"
#include "globals.h"

#include "vm/vmmap.h"
#include "vm/shadow.h"
#include "vm/anon.h"

#include "proc/proc.h"

#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"
#include "util/printf.h"

#include "fs/vnode.h"
#include "fs/file.h"
#include "fs/fcntl.h"
#include "fs/vfs_syscall.h"

#include "mm/slab.h"
#include "mm/page.h"
#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/mmobj.h"

#define min(a, b) ((a) < (b) ? (a) : (b))

static slab_allocator_t *vmmap_allocator;
static slab_allocator_t *vmarea_allocator;

void vmmap_init(void)
{
        vmmap_allocator = slab_allocator_create("vmmap", sizeof(vmmap_t));
        KASSERT(NULL != vmmap_allocator && "failed to create vmmap allocator!");
        vmarea_allocator = slab_allocator_create("vmarea", sizeof(vmarea_t));
        KASSERT(NULL != vmarea_allocator && "failed to create vmarea allocator!");
}

vmarea_t *
vmarea_alloc(void)
{
        vmarea_t *newvma = (vmarea_t *)slab_obj_alloc(vmarea_allocator);
        if (newvma)
        {
                newvma->vma_vmmap = NULL;
        }
        return newvma;
}

void vmarea_free(vmarea_t *vma)
{
        KASSERT(NULL != vma);
        slab_obj_free(vmarea_allocator, vma);
}

/* a debugging routine: dumps the mappings of the given address space. */
size_t
vmmap_mapping_info(const void *vmmap, char *buf, size_t osize)
{
        KASSERT(0 < osize);
        KASSERT(NULL != buf);
        KASSERT(NULL != vmmap);

        vmmap_t *map = (vmmap_t *)vmmap;
        vmarea_t *vma;
        ssize_t size = (ssize_t)osize;

        int len = snprintf(buf, size, "%21s %5s %7s %8s %10s %12s\n",
                           "VADDR RANGE", "PROT", "FLAGS", "MMOBJ", "OFFSET",
                           "VFN RANGE");

        list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink)
        {
                size -= len;
                buf += len;
                if (0 >= size)
                {
                        goto end;
                }

                len = snprintf(buf, size,
                               "%#.8x-%#.8x  %c%c%c  %7s 0x%p %#.5x %#.5x-%#.5x\n",
                               vma->vma_start << PAGE_SHIFT,
                               vma->vma_end << PAGE_SHIFT,
                               (vma->vma_prot & PROT_READ ? 'r' : '-'),
                               (vma->vma_prot & PROT_WRITE ? 'w' : '-'),
                               (vma->vma_prot & PROT_EXEC ? 'x' : '-'),
                               (vma->vma_flags & MAP_SHARED ? " SHARED" : "PRIVATE"),
                               vma->vma_obj, vma->vma_off, vma->vma_start, vma->vma_end);
        }
        list_iterate_end();

end:
        if (size <= 0)
        {
                size = osize;
                buf[osize - 1] = '\0';
        }
        /*
        KASSERT(0 <= size);
        if (0 == size) {
                size++;
                buf--;
                buf[0] = '\0';
        }
        */
        return osize - size;
}

/* Create a new vmmap, which has no vmareas and does
 * not refer to a process. */
vmmap_t *
vmmap_create(void)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_create");

        /* Create a new vmmap, which has no vmareas and does
         * not refer to a process. */

        vmmap_t *new_vmmap = (vmmap_t *)slab_obj_alloc(vmmap_allocator);
        if (new_vmmap)
        {
                list_init(&new_vmmap->vmm_list);
                new_vmmap->vmm_proc = NULL;
        }
        return new_vmmap;

        // return NULL;
}

/* Removes all vmareas from the address space and frees the
 * vmmap struct. */
void vmmap_destroy(vmmap_t *map)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_destroy");
        /* Removes all vmareas from the address space and frees the
         * vmmap struct. */

        KASSERT(NULL != map);
        vmarea_t *vma;
        list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink)
        {
                list_remove(&vma->vma_plink);
                vma->vma_vmmap = NULL;
                vmarea_free(vma);
        }
        list_iterate_end();
        slab_obj_free(vmmap_allocator, map);
}

/* Add a vmarea to an address space. Assumes (i.e. asserts to some extent)
 * the vmarea is valid.  This involves finding where to put it in the list
 * of VM areas, and adding it. Don't forget to set the vma_vmmap for the
 * area. */
void vmmap_insert(vmmap_t *map, vmarea_t *newvma)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_insert");

        /* Add a vmarea to an address space.
                Assumes (i.e. asserts to some extent) the vmarea is valid.
        * This involves finding where to put it in the list of VM areas, and adding it
        * Don't forget to set the vma_vmmap for the area.
        */

        KASSERT(NULL != map && NULL != newvma);
        KASSERT(NULL == newvma->vma_vmmap);

        int vma_inserted = 0;

        newvma->vma_vmmap = map;

        vmarea_t *vma;
        list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink)
        {
                if (vma->vma_start > newvma->vma_start)
                {
                        list_insert_before(&vma->vma_plink, &newvma->vma_plink);
                        vma_inserted = 1;
                }
        }
        list_iterate_end();

        if (!vma_inserted)
                list_insert_tail(&map->vmm_list, &newvma->vma_plink);

        return;
}

/* Find a contiguous range of free virtual pages of length npages in
 * the given address space. Returns starting vfn for the range,
 * without altering the map. Returns -1 if no such range exists.
 *
 * Your algorithm should be first fit. If dir is VMMAP_DIR_HILO, you
 * should find a gap as high in the address space as possible; if dir
 * is VMMAP_DIR_LOHI, the gap should be as low as possible. */
int vmmap_find_range(vmmap_t *map, uint32_t npages, int dir)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_find_range");
        // return -1;

        /* Find a contiguous range of free virtual pages of length npages in the given address space.
         * Returns starting vfn for the range, without altering the map. Returns -1 if no such range exists.
         *
         *
         * Your algorithm should be first fit. If dir is VMMAP_DIR_HILO, you
         * should find a gap as high in the address space as possible; if dir
         * is VMMAP_DIR_LOHI, the gap should be as low as possible. */

        KASSERT(NULL != map);
        KASSERT(0 < npages);

        uint32_t vfn = 0;
        uint32_t start_vfn = 0;
        uint32_t end_vfn = 0;
        int found = 0;

        vmarea_t *vma;

        if (list_empty(&map->vmm_list))
        {
                // Handle the case where the list is empty.
                // For example, if the entire address space is available, return the start of the address space.
                return 0;
        }

        if (dir == VMMAP_DIR_HILO)
        {
                list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink)
                {
                        end_vfn = vma->vma_start;
                        if (end_vfn - start_vfn >= npages)
                        {
                                found = 1;
                                break;
                        }
                        start_vfn = vma->vma_end;
                }
                list_iterate_end();
        }
        else if (dir == VMMAP_DIR_LOHI)
        {
                list_iterate_reverse(&map->vmm_list, vma, vmarea_t, vma_plink)
                {
                        start_vfn = vma->vma_end;
                        if (start_vfn - end_vfn >= npages)
                        {
                                found = 1;
                                break;
                        }
                        end_vfn = vma->vma_start;
                }
                list_iterate_end();
        }

        if (!found)
        {
                if (dir == VMMAP_DIR_HILO)
                {
                        if (end_vfn - start_vfn >= npages)
                        {
                                found = 1;
                        }
                }
                else if (dir == VMMAP_DIR_LOHI)
                {
                        if (start_vfn - end_vfn >= npages)
                        {
                                found = 1;
                        }
                }
        }

        else
        {
                if (dir == VMMAP_DIR_HILO)
                {
                        vfn = end_vfn - npages;
                }
                else if (dir == VMMAP_DIR_LOHI)
                {
                        vfn = start_vfn;
                }
        }

        if (found)
        {
                return vfn;
        }
        else
        {
                return -1;
        }
}

/* Find the vm_area that vfn lies in. Simply scan the address space
 * looking for a vma whose range covers vfn. If the page is unmapped,
 * return NULL. */
vmarea_t *
vmmap_lookup(vmmap_t *map, uint32_t vfn)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_lookup");
        // return NULL;

        /* Find the vm_area that vfn lies in. Simply scan the address space
         * looking for a vma whose range covers vfn. If the page is unmapped,
         * return NULL. */

        KASSERT(NULL != map);

        vmarea_t *vma;
        list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink)
        {
                if (vma->vma_start <= vfn && vma->vma_end > vfn)
                {
                        return vma;
                }
        }
        list_iterate_end();

        return NULL;
}

/* Allocates a new vmmap containing a new vmarea for each area in the
 * given map. The areas should have no mmobjs set yet. Returns pointer
 * to the new vmmap on success, NULL on failure. This function is
 * called when implementing fork(2). */
vmmap_t *
vmmap_clone(vmmap_t *map)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_clone");
        // return NULL;

        /* Allocates a new vmmap containing a new vmarea for each area in the
         * given map. The areas should have no mmobjs set yet. Returns pointer
         * to the new vmmap on success, NULL on failure. This function is
         * called when implementing fork(2). */

        KASSERT(NULL != map);

        vmmap_t *new_vmmap = vmmap_create();
        if (new_vmmap)
        {
                vmarea_t *vma;
                list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink)
                {
                        vmarea_t *new_vma = vmarea_alloc();
                        if (new_vma)
                        {
                                new_vma->vma_start = vma->vma_start;
                                new_vma->vma_end = vma->vma_end;
                                new_vma->vma_off = vma->vma_off;
                                new_vma->vma_prot = vma->vma_prot;
                                new_vma->vma_flags = vma->vma_flags;
                                new_vma->vma_obj = NULL;
                                vmmap_insert(new_vmmap, new_vma);
                        }
                        else
                        {
                                vmmap_destroy(new_vmmap);
                                return NULL;
                        }
                }
                list_iterate_end();
                return new_vmmap;
        }
        return NULL;
}

/* Insert a mapping into the map starting at lopage for npages pages.
 * If lopage is zero, we will find a range of virtual addresses in the
 * process that is big enough, by using vmmap_find_range with the same
 * dir argument.  If lopage is non-zero and the specified region
 * contains another mapping that mapping should be unmapped.
 *
 * If file is NULL an anon mmobj will be used to create a mapping
 * of 0's.  If file is non-null that vnode's file will be mapped in
 * for the given range.  Use the vnode's mmap operation to get the
 * mmobj for the file; do not assume it is file->vn_obj. Make sure all
 * of the area's fields except for vma_obj have been set before
 * calling mmap.
 *
 * If MAP_PRIVATE is specified set up a shadow object for the mmobj.
 *
 * All of the input to this function should be valid (KASSERT!).
 * See mmap(2) for for description of legal input.
 * Note that off should be page aligned.
 *
 * Be very careful about the order operations are performed in here. Some
 * operation are impossible to undo and should be saved until there
 * is no chance of failure.
 *
 * If 'new' is non-NULL a pointer to the new vmarea_t should be stored in it.
 */
int vmmap_map(vmmap_t *map, vnode_t *file, uint32_t lopage, uint32_t npages,
              int prot, int flags, off_t off, int dir, vmarea_t **new)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_map");
        // return -1;

        /*
        - Check the lopage argument: If lopage is zero, you need to find a range of virtual addresses in the process that is big enough.
        You can use vmmap_find_range with the same dir argument for this. If lopage is non-zero and the specified region contains another mapping, that mapping should be unmapped.

        - Create a memory object (mmobj): If file is NULL, an anonymous memory object should be used to create a mapping of 0's.
        If file is non-null, you should use the vnode's mmap operation to get the memory object for the file. Do not assume it is file->vn_obj.

        - Set up the area's fields: Make sure all of the area's fields except for vma_obj have been set before calling mmap.

        - Handle MAP_PRIVATE: If MAP_PRIVATE is specified, set up a shadow object for the memory object.

        - Perform operations carefully: Some operations are impossible to undo and should be saved until there is no chance of failure.

        - Store the new vmarea_t: If 'new' is non-NULL, a pointer to the new vmarea_t should be stored in it.

        - Remember to use KASSERT to validate all of the input to this function. The off argument should be page aligned. Be very careful about the order operations are performed in here.
        */

        KASSERT(NULL != map);
        KASSERT(0 < npages);
        KASSERT(PAGE_ALIGNED(off));
        KASSERT(!(flags & MAP_FIXED) || (flags & MAP_FIXED && lopage != 0));
        KASSERT(!(flags & MAP_ANON) || (flags & MAP_ANON && file == NULL));
        KASSERT(!(flags & MAP_PRIVATE) || (flags & MAP_PRIVATE && file != NULL));
        KASSERT(!(flags & MAP_SHARED) || (flags & MAP_SHARED && file != NULL));

        int vfn = lopage;
        if (vfn == 0)
        {
                vfn = vmmap_find_range(map, npages, dir);
                if (vfn == -1)
                {
                        return -1;
                }
        }
        else
        {
                vmarea_t *vma = vmmap_lookup(map, vfn);
                if (vma != NULL)
                {
                        if (vma->vma_start <= (uint32_t)vfn && vma->vma_end >= (uint32_t)(vfn + npages))
                        {
                                vmmap_remove(map, vfn, npages);
                        }
                        else
                        {
                                return -1;
                        }
                }
        }

        vmarea_t *new_vma = vmarea_alloc();
        if (new_vma == NULL)
                return -1;

        new_vma->vma_start = vfn;
        new_vma->vma_end = vfn + npages;
        new_vma->vma_off = off;
        new_vma->vma_prot = prot;
        new_vma->vma_flags = flags;
        new_vma->vma_obj = NULL;

        if (file == NULL)
        {
                new_vma->vma_obj = anon_create();
        }
        else
        {
                mmobj_t *mmobj = NULL;
                int ret = file->vn_ops->mmap(file, new_vma, &mmobj);
                if (ret < 0)
                {
                        vmarea_free(new_vma);
                        return -1;
                }
                new_vma->vma_obj = mmobj;
        }

        if (flags & MAP_PRIVATE)
        {
                mmobj_t *shadow = shadow_create();
                if (shadow == NULL)
                {
                        vmarea_free(new_vma);
                        return -1;
                }
                shadow->mmo_shadowed = new_vma->vma_obj;
                new_vma->vma_obj = shadow;
        }

        vmmap_insert(map, new_vma);

        if (new != NULL)
        {
                *new = new_vma;
        }

        return 0;
}

/*
 * We have no guarantee that the region of the address space being
 * unmapped will play nicely with our list of vmareas.
 *
 * You must iterate over each vmarea that is partially or wholly covered
 * by the address range [addr ... addr+len). The vm-area will fall into one
 * of four cases, as illustrated below:
 *
 * key:
 *          [             ]   Existing VM Area
 *        *******             Region to be unmapped
 *
 * Case 1:  [   ******    ]
 * The region to be unmapped lies completely inside the vmarea. We need to
 * split the old vmarea into two vmareas. be sure to increment the
 * reference count to the file associated with the vmarea.
 *
 * Case 2:  [      *******]**
 * The region overlaps the end of the vmarea. Just shorten the length of
 * the mapping.
 *
 * Case 3: *[*****        ]
 * The region overlaps the beginning of the vmarea. Move the beginning of
 * the mapping (remember to update vma_off), and shorten its length.
 *
 * Case 4: *[*************]**
 * The region completely contains the vmarea. Remove the vmarea from the
 * list.
 */
int vmmap_remove(vmmap_t *map, uint32_t lopage, uint32_t npages)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_remove");
        // return -1;

        /*
         * We have no guarantee that the region of the address space being
         * unmapped will play nicely with our list of vmareas.
         *
         * You must iterate over each vmarea that is partially or wholly covered
         * by the address range [addr ... addr+len). The vm-area will fall into one
         * of four cases, as illustrated below:
         *
         * key:
         *          [             ]   Existing VM Area
         *        *******             Region to be unmapped
         *
         * Case 1:  [   ******    ]
         * The region to be unmapped lies completely inside the vmarea. We need to
         * split the old vmarea into two vmareas. be sure to increment the
         * reference count to the file associated with the vmarea.
         *
         * Case 2:  [      *******]**
         * The region overlaps the end of the vmarea. Just shorten the length of
         * the mapping.
         *
         * Case 3: *[*****        ]
         * The region overlaps the beginning of the vmarea. Move the beginning of
         * the mapping (remember to update vma_off), and shorten its length.
         *
         * Case 4: *[*************]**
         * The region completely contains the vmarea. Remove the vmarea from the
         * list.
         */

        KASSERT(NULL != map);
        KASSERT(0 < npages);

        uint32_t start_vfn = lopage;
        uint32_t end_vfn = lopage + npages;

        vmarea_t *vma;

        list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink)
        {

                // case 1: [   ******    ]
                if (vma->vma_start < start_vfn && vma->vma_end > end_vfn)
                {
                        vmarea_t *new_vma = vmarea_alloc();
                        if (new_vma == NULL)
                        {
                                return -1;
                        }
                        new_vma->vma_start = end_vfn;
                        new_vma->vma_end = vma->vma_end;
                        new_vma->vma_off = vma->vma_off + (end_vfn - vma->vma_start);
                        new_vma->vma_prot = vma->vma_prot;
                        new_vma->vma_flags = vma->vma_flags;
                        new_vma->vma_obj = vma->vma_obj;
                        vma->vma_end = start_vfn;
                        list_insert_before(vma->vma_plink.l_next, &new_vma->vma_plink);
                }

                // case 2: [      *******]**
                else if (vma->vma_start < start_vfn && vma->vma_end > start_vfn)
                {
                        vma->vma_end = start_vfn;
                }

                // case 3: *[*****        ]
                else if (vma->vma_start < end_vfn && vma->vma_end > end_vfn)
                {
                        vma->vma_off += end_vfn - vma->vma_start;
                        vma->vma_start = end_vfn;
                }

                // case 4: *[*************]**
                else if (vma->vma_start >= start_vfn && vma->vma_end <= end_vfn)
                {
                        list_remove(&vma->vma_plink);
                        vmarea_free(vma);
                }
        }
        list_iterate_end();

        return 0;
}

/*
 * Returns 1 if the given address space has no mappings for the
 * given range, 0 otherwise.
 */
int vmmap_is_range_empty(vmmap_t *map, uint32_t startvfn, uint32_t npages)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_is_range_empty");
        // return 0;

        /*
         * Returns 1 if the given address space has no mappings for the
         * given range, 0 otherwise.
         */

        KASSERT(NULL != map);
        KASSERT(0 < npages);

        uint32_t endvfn = startvfn + npages;
        vmarea_t *vma;

        list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink)
        {
                if ((vma->vma_start < endvfn && vma->vma_end > startvfn))
                {
                        return 0;
                }
        }
        list_iterate_end();

        return 1;
}

/* Read into 'buf' from the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do so, you will want to find the vmareas
 * to read from, then find the pframes within those vmareas corresponding
 * to the virtual addresses you want to read, and then read from the
 * physical memory that pframe points to. You should not check permissions
 * of the areas. Assume (KASSERT) that all the areas you are accessing exist.
 * Returns 0 on success, -errno on error.
 */
int vmmap_read(vmmap_t *map, const void *vaddr, void *buf, size_t count)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_read");
        // return 0;

        /*
        Find the vmareas to read from: You need to locate the virtual memory areas (vmareas) that contain the data you want to read. This will involve searching through the vmareas in the 'map' to find those that cover the range from 'vaddr' to 'vaddr + count'.

Find the pframes within those vmareas: Once you've found the relevant vmareas, you need to locate the physical frames (pframes) that correspond to the virtual addresses you want to read. This will involve translating the virtual addresses to physical addresses.

Read from the physical memory that pframe points to: After you've found the pframes, you can read the data from the physical memory they point to. This will involve copying the data from the physical memory into 'buf'.

Return 0 on success, -errno on error: If all the above steps are successful, you should return 0. If any error occurs (for example, if a vmarea or pframe cannot be found), you should return an appropriate error code.

Assume all the areas you are accessing exist: You should not check the permissions of the areas you are accessing. Instead, you should assume that all the areas you need to access do exist. You can use the KASSERT macro to enforce this assumption.
        */

        KASSERT(NULL != map);
        KASSERT(NULL != vaddr);
        KASSERT(NULL != buf);
        KASSERT(0 < count);

        uint32_t vfn = ADDR_TO_PN(vaddr);
        uint32_t end_vfn = ADDR_TO_PN((char *)vaddr + count);
        uint32_t offset = PAGE_OFFSET(vaddr);
        size_t bytes_remaining = count;

        vmarea_t *vma;
        pframe_t *pf;
        int ret;

        while (vfn < end_vfn)
        {
                vma = vmmap_lookup(map, vfn);
                KASSERT(NULL != vma);

                uint32_t vma_off = vma->vma_off;
                uint32_t vma_vfn = vfn - vma->vma_start + vma_off;

                ret = pframe_get(vma->vma_obj, vma_vfn, &pf);
                if (ret < 0)
                {
                        return ret; // propagate the error
                }
                KASSERT(NULL != pf);

                pframe_pin(pf);

                void *paddr = PN_TO_ADDR(pf->pf_pagenum);
                size_t bytes_to_copy = min(PAGE_SIZE - offset, bytes_remaining);

                memcpy(buf, (char *)paddr + offset, bytes_to_copy);

                pframe_unpin(pf);

                vfn++;
                buf = (char *)buf + bytes_to_copy;
                bytes_remaining -= bytes_to_copy;
                offset = 0; // offset is only for the first page
        }

        return 0;
}

/* Write from 'buf' into the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do this, you will need to find the correct
 * vmareas to write into, then find the correct pframes within those vmareas,
 * and finally write into the physical addresses that those pframes correspond
 * to. You should not check permissions of the areas you use. Assume (KASSERT)
 * that all the areas you are accessing exist. Remember to dirty pages!
 * Returns 0 on success, -errno on error.
 */
int vmmap_write(vmmap_t *map, void *vaddr, const void *buf, size_t count)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_write");
        // return 0;

        /*
        function vmmap_write(map, vaddr, buf, count)
                validate inputs
                calculate start and end page numbers
                for each page in the range
                        look up vmarea
                        calculate page number within vmarea
                        get pframe
                        pin pframe
                        calculate physical address
                        determine number of bytes to write
                        copy data from buffer to physical address
                        mark pframe as dirty
                        unpin pframe
                        move to next page and adjust buffer pointer and remaining bytes
                return 0
        */

        KASSERT(NULL != map);
        KASSERT(NULL != vaddr);
        KASSERT(NULL != buf);
        KASSERT(0 < count);

        uint32_t vfn = ADDR_TO_PN(vaddr);
        uint32_t end_vfn = ADDR_TO_PN((char *)vaddr + count);
        uint32_t offset = PAGE_OFFSET(vaddr);
        size_t bytes_remaining = count;

        vmarea_t *vma;
        pframe_t *pf;
        int ret;

        while (vfn < end_vfn)
        {
                vma = vmmap_lookup(map, vfn);
                KASSERT(NULL != vma);

                uint32_t vma_off = vma->vma_off;
                uint32_t vma_vfn = vfn - vma->vma_start + vma_off;

                ret = pframe_get(vma->vma_obj, vma_vfn, &pf);
                if (ret < 0)
                {
                        return ret; // propagate the error
                }
                KASSERT(NULL != pf);

                pframe_pin(pf);

                void *paddr = PN_TO_ADDR(pf->pf_pagenum);
                size_t bytes_to_copy = min(PAGE_SIZE - offset, bytes_remaining);

                memcpy((char *)paddr + offset, buf, bytes_to_copy);

                pframe_set_dirty(pf);
                pframe_unpin(pf);

                vfn++;
                buf = (char *)buf + bytes_to_copy;
                bytes_remaining -= bytes_to_copy;
                offset = 0; // offset is only for the first page
        }

        return 0;
}
