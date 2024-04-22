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

/*
 *  FILE: open.c
 *  AUTH: mcc | jal
 *  DESC:
 *  DATE: Mon Apr  6 19:27:49 1998
 */

#include "globals.h"
#include "errno.h"
#include "fs/fcntl.h"
#include "util/string.h"
#include "util/printf.h"
#include "fs/vfs.h"
#include "fs/vnode.h"
#include "fs/file.h"
#include "fs/vfs_syscall.h"
#include "fs/open.h"
#include "fs/stat.h"
#include "util/debug.h"

/* find empty index in p->p_files[] */
int get_empty_fd(proc_t *p)
{
        int fd;

        for (fd = 0; fd < NFILES; fd++)
        {
                if (!p->p_files[fd])
                        return fd;
        }

        dbg(DBG_ERROR | DBG_VFS, "ERROR: get_empty_fd: out of file descriptors "
                                 "for pid %d\n",
            curproc->p_pid);
        return -EMFILE;
}

/*
 * There a number of steps to opening a file:
 *      1. Get the next empty file descriptor.
 *      2. Call fget to get a fresh file_t.
 *      3. Save the file_t in curproc's file descriptor table.
 *      4. Set file_t->f_mode to OR of FMODE_(READ|WRITE|APPEND) based on
 *         oflags, which can be O_RDONLY, O_WRONLY or O_RDWR, possibly OR'd with
 *         O_APPEND.
 *      5. Use open_namev() to get the vnode for the file_t.
 *      6. Fill in the fields of the file_t.
 *      7. Return new fd.
 *
 * If anything goes wrong at any point (specifically if the call to open_namev
 * fails), be sure to remove the fd from curproc, fput the file_t and return an
 * error.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EINVAL
 *        oflags is not valid.
 *      o EMFILE
 *        The process already has the maximum number of files open.
 *      o ENOMEM
 *        Insufficient kernel memory was available.
 *      o ENAMETOOLONG
 *        A component of filename was too long.
 *      o ENOENT
 *        O_CREAT is not set and the named file does not exist.  Or, a
 *        directory component in pathname does not exist.
 *      o EISDIR
 *        pathname refers to a directory and the access requested involved
 *        writing (that is, O_WRONLY or O_RDWR is set).
 *      o ENXIO
 *        pathname refers to a device special file and no corresponding device
 *        exists.
 */

/*
int do_open(const char *filename, int oflags)
{
        dbg(DBG_PRINT, "(GRADING2A)\n");
        // 1. Get the next empty file descriptor.
        int fileDis = get_empty_fd(curproc);
        if (fileDis < 0)
        { // Check for no available file descriptor
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EMFILE;
        }

        // 2. Allocate a new file_t structure.
        dbg(DBG_PRINT, "(GRADING2A)\n");

        file_t *newFile = fget(-1); // If fget fails, it should return NULL or handle internally
        if (!newFile)
        { // Check for failure in allocating a file_t structure
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -ENOMEM;
        }

        // Initialize newFile's f_mode to 0
        dbg(DBG_PRINT, "(GRADING2A)\n");
        newFile->f_mode = 0;

        // Combine checks for flag validity and setting of newFile->f_mode
        if ((oflags & (O_WRONLY | O_RDWR)) == (O_WRONLY | O_RDWR))
        {
                // Invalid combination of flags, return error
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EINVAL;
        }
        else
        {
                // Valid flag combinations
                dbg(DBG_PRINT, "(GRADING2A)\n");
                if (oflags == O_RDONLY)
                {
                        dbg(DBG_PRINT, "(GRADING2B)\n");
                        newFile->f_mode |= FMODE_READ;
                }
                else if (oflags & O_RDWR)
                {
                        dbg(DBG_PRINT, "(GRADING2A)\n");
                        newFile->f_mode |= FMODE_READ | FMODE_WRITE;
                }
                else if (oflags & O_WRONLY)
                {
                        dbg(DBG_PRINT, "(GRADING2B)\n");
                        newFile->f_mode |= FMODE_WRITE;
                }

                if (oflags & O_APPEND)
                {
                        dbg(DBG_PRINT, "(GRADING2B)\n");
                        newFile->f_mode |= FMODE_APPEND;
                }
        }

        dbg(DBG_PRINT, "(GRADING2A)\n");

        // 3. Attempt to open the vnode for the given filename
        vnode_t *tvNode = NULL;
        int OpenStatus = open_namev(filename, oflags, &tvNode, NULL);
        dbg(DBG_PRINT, "(GRADING2A)\n");

        if (OpenStatus < 0)
        {                      // If opening the vnode fails, clean up
                fput(newFile); // Release the file_t structure
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return OpenStatus;
        }

        // Check if attempting to write to a directory
        if (S_ISDIR(tvNode->vn_mode) && (oflags & (O_WRONLY | O_RDWR)))
        {
                vput(tvNode);  // Decrease vnode reference count
                fput(newFile); // Release the file_t structure
                dbg(DBG_PRINT, "(GRADING2B)\n");

                return -EISDIR;
        }

        // Setup the remaining file_t structure fields
        dbg(DBG_PRINT, "(GRADING2A)\n");

        newFile->f_vnode = tvNode;
        newFile->f_pos = 0;

        // Save the file_t in the process's file descriptor table
        curproc->p_files[fileDis] = newFile;

        // Return the new file descriptor
        dbg(DBG_PRINT, "(GRADING2A)\n");

        return fileDis;
}
*/

// Ridheesh
int do_open(const char *filename, int oflags)
{
        // NOT_YET_IMPLEMENTED("VFS: do_open");

        /* No invalid combinations of O_RDONLY, O_WRONLY, and O_RDWR.  Since
         * O_RDONLY is stupidly defined as 0, the only invalid possible
         * combination is O_WRONLY|O_RDWR.
         *                                              ----vfstest_open()
         * */
        // check oflags
        if ((oflags & O_WRONLY) && (oflags & O_RDWR))
        {
                return -EINVAL;
        }

        // 1. Get the next empty file descriptor.
        int newfd = get_empty_fd(curproc);
        // if newfd < 0, get_empty_fd falis
        // case: -EMFILE
        if (newfd < 0)
        {
                return newfd;
        }

        // 2. Call fget to get a fresh file_t.
        file_t *newFile = fget(-1);

        // 3. Save the file_t in curproc's file descriptor table.
        curproc->p_files[newfd] = newFile;

        //  4. Set file_t->f_mode to OR of FMODE_(READ|WRITE|APPEND) based on
        //  oflags, which can be O_RDONLY, O_WRONLY or O_RDWR, possibly OR'd with
        //  O_APPEND.
        newFile->f_mode = 0;

        if (oflags == O_RDONLY)
        {
                newFile->f_mode |= FMODE_READ;
        }
        if (oflags & O_WRONLY)
        {
                newFile->f_mode |= FMODE_WRITE;
        }
        if (oflags & O_RDWR)
        {
                newFile->f_mode |= (FMODE_READ | FMODE_WRITE);
        }
        if (oflags & O_APPEND)
        {
                newFile->f_mode |= FMODE_APPEND;
        }

        // 5. Use open_namev() to get the vnode for the file_t.
        vnode_t *resVNode = NULL;
        int code = open_namev(filename, oflags, &resVNode, NULL);
        // if code < 0, open_namev() fails
        // case: -ENOENT, -ENOTDIR, and -ENAMETOOLONG
        if (code < 0)
        {
                // fput the newFile
                fput(newFile);
                // remove the newfd from curproc
                curproc->p_files[newfd] = NULL;
                return code;
        }

        // check -EISDIR
        if (S_ISDIR(resVNode->vn_mode) && ((oflags & O_WRONLY) || (oflags & O_RDWR)))
        {
                // decrease resVNode vn_refcount
                vput(resVNode);
                // fput the newFile
                fput(newFile);
                // remove the newfd from curproc
                curproc->p_files[newfd] = NULL;
                return -EISDIR;
        }

        // even if resVNode is a device special file
        // there is no need to check -ENXIO
        // because code >= 0 means resVNode exists

        // 6. Fill in the fields of the file_t.
        newFile->f_pos = 0;
        newFile->f_vnode = resVNode;

        // 7. Return new fd.
        return newfd;
}