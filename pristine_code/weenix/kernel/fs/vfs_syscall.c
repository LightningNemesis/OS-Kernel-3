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
 *  FILE: vfs_syscall.c
 *  AUTH: mcc | jal
 *  DESC:
 *  DATE: Wed Apr  8 02:46:19 1998
 *  $Id: vfs_syscall.c,v 1.2 2018/05/27 03:57:26 cvsps Exp $
 */

#include "kernel.h"
#include "errno.h"
#include "globals.h"
#include "fs/vfs.h"
#include "fs/file.h"
#include "fs/vnode.h"
#include "fs/vfs_syscall.h"
#include "fs/open.h"
#include "fs/fcntl.h"
#include "fs/lseek.h"
#include "mm/kmalloc.h"
#include "util/string.h"
#include "util/printf.h"
#include "fs/stat.h"
#include "util/debug.h"

/*
 * Syscalls for vfs. Refer to comments or man pages for implementation.
 * Do note that you don't need to set errno, you should just return the
 * negative error code.
 */

/* To read a file:
 *      o fget(fd)
 *      o call its virtual read vn_op
 *      o update f_pos
 *      o fput() it
 *      o return the number of bytes read, or an error
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not a valid file descriptor or is not open for reading.
 *      o EISDIR
 *        fd refers to a directory.
 *
 * In all cases, be sure you do not leak file refcounts by returning before
 * you fput() a file that you fget()'ed.
 */
int do_read(int fd, void *buf, size_t nbytes)
{
        dbg(DBG_PRINT, "(GRADING2A)\n");

        if (fd < 0 || fd >= NFILES || !curproc->p_files[fd])
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF; // Invalid file descriptor
        }

        file_t *fileObj = fget(fd); // Retrieve the file object associated with the file descriptor
        if (!fileObj)
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF; // File descriptor not associated with an open file
        }

        if (fileObj->f_vnode->vn_ops->read == NULL)
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");

                fput(fileObj);
                return -EISDIR;
        }

        if (!(fileObj->f_mode & FMODE_READ))
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                fput(fileObj); // Release the file object
                return -EBADF; // File not open for reading
        }
        dbg(DBG_PRINT, "(GRADING2A)\n");

        int result = fileObj->f_vnode->vn_ops->read(fileObj->f_vnode, fileObj->f_pos, buf, nbytes);
        fileObj->f_pos += (result > 0) ? result : 0; // Advance the file position only if read was successful

        dbg(DBG_PRINT, "(GRADING2A)\n");
        fput(fileObj); // Always release the file object when done
        return result; // Return the result of the read operation
}

/* Very similar to do_read.  Check f_mode to be sure the file is writable.  If
 * f_mode & FMODE_APPEND, do_lseek() to the end of the file, call the write
 * vn_op, and fput the file.  As always, be mindful of refcount leaks.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not a valid file descriptor or is not open for writing.
 */
int do_write(int fd, const void *buf, size_t nbytes)
{
        dbg(DBG_PRINT, "(GRADING2A)\n");

        if (fd < 0 || fd >= NFILES || !curproc->p_files[fd])
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF;
        }
        dbg(DBG_PRINT, "(GRADING2A)\n");

        file_t *file = fget(fd);
        int res;
        if (file != NULL)
        {
                dbg(DBG_PRINT, "(GRADING2A)\n");

                if ((file->f_mode & (FMODE_WRITE | FMODE_APPEND)) == 0)
                {
                        dbg(DBG_PRINT, "(GRADING2B)\n");

                        fput(file);
                        return -EBADF;
                }
                if (!S_ISDIR(file->f_vnode->vn_mode) && file->f_vnode->vn_ops->write != NULL)
                {
                        dbg(DBG_PRINT, "(GRADING2A)\n");

                        if (file->f_mode & FMODE_APPEND)
                        {
                                dbg(DBG_PRINT, "(GRADING2B)\n");

                                do_lseek(fd, 0, SEEK_END);
                        }
                        dbg(DBG_PRINT, "(GRADING2A)\n");

                        res = file->f_vnode->vn_ops->write(file->f_vnode, file->f_pos, buf, nbytes);
                        file->f_pos += res;

                        KASSERT((S_ISCHR(file->f_vnode->vn_mode)) ||
                                (S_ISBLK(file->f_vnode->vn_mode)) ||
                                ((S_ISREG(file->f_vnode->vn_mode)) && (file->f_pos <= file->f_vnode->vn_len))); /* cursor must not go past end of file for these file types */
                        dbg(DBG_PRINT, "(GRADING2A 3.a)\n");
                        fput(file);
                }
        }
        else
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");

                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF;
        }
        dbg(DBG_PRINT, "(GRADING2A)\n");

        return res;
}

/*
 * Zero curproc->p_files[fd], and fput() the file. Return 0 on success
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd isn't a valid open file descriptor.
 */
int do_close(int fd)
{
        // Check if the file descriptor is within the acceptable range.
        if (fd < 0 || fd >= NFILES || !curproc->p_files[fd])
        {
                dbg(DBG_PRINT, "(GRADING2B)\n"); // Log the event for debugging purposes.
                return -EBADF;                   // Invalid file descriptor.
        }
        else
        {
                dbg(DBG_PRINT, "(GRADING2A)\n");
                if (curproc->p_files[fd])
                {
                        dbg(DBG_PRINT, "(GRADING2A)\n");

                        // Close the file by releasing its resources.
                        fput(curproc->p_files[fd]);
                        curproc->p_files[fd] = NULL; // Reset the file descriptor for future use.

                        dbg(DBG_PRINT, "(GRADING2A)\n");
                        return 0; // Successfully closed the file.
                }
                else
                {
                        dbg(DBG_PRINT, "(GRADING2B)\n"); // Log the event for debugging purposes.
                        return -EBADF;                   // No open file associated with this file descriptor.
                }
        }
}

/* To dup a file:
 *      o fget(fd) to up fd's refcount
 *      o get_empty_fd()
 *      o point the new fd to the same file_t* as the given fd
 *      o return the new file descriptor
 *
 * Don't fput() the fd unless something goes wrong.  Since we are creating
 * another reference to the file_t*, we want to up the refcount.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd isn't an open file descriptor.
 *      o EMFILE
 *        The process already has the maximum number of file descriptors open
 *        and tried to open a new one.
 */
int do_dup(int fd)
{
        dbg(DBG_PRINT, "(GRADING2B)\n");

        if (fd < 0 || fd >= NFILES || !curproc->p_files[fd])
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF;
        }

        file_t *fDup = fget(fd);

        int nfd = get_empty_fd(curproc);
        // if (nfd < 0)
        // {
        //         fput(fDup);
        //         dbg(DBG_PRINT, "(GRADING2B)\n");
        //         return -EMFILE;
        // }
        curproc->p_files[nfd] = fDup;
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return nfd;
}

/* Same as do_dup, but insted of using get_empty_fd() to get the new fd,
 * they give it to us in 'nfd'.  If nfd is in use (and not the same as ofd)
 * do_close() it first.  Then return the new file descriptor.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        ofd isn't an open file descriptor, or nfd is out of the allowed
 *        range for file descriptors.
 */
int do_dup2(int ofd, int nfd)
{
        dbg(DBG_PRINT, "(GRADING2B)\n");
        if (ofd < 0 || ofd > NFILES || nfd >= NFILES)
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF;
        }

        if (!curproc->p_files[ofd])
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF;
        }

        file_t *fDup2 = fget(ofd);

        if (nfd == ofd)
        {
                fput(fDup2);
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }

        else
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                if (curproc->p_files[nfd])
                {
                        dbg(DBG_PRINT, "(GRADING2B)\n");
                        do_close(nfd);
                }
        }
        curproc->p_files[nfd] = fDup2;
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return nfd;
}

/*
 * This routine creates a special file of the type specified by 'mode' at
 * the location specified by 'path'. 'mode' should be one of S_IFCHR or
 * S_IFBLK (you might note that mknod(2) normally allows one to create
 * regular files as well-- for simplicity this is not the case in Weenix).
 * 'devid', as you might expect, is the device identifier of the device
 * that the new special file should represent.
 *
 * You might use a combination of dir_namev, lookup, and the fs-specific
 * mknod (that is, the containing directory's 'mknod' vnode operation).
 * Return the result of the fs-specific mknod, or an error.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EINVAL
 *        mode requested creation of something other than a device special
 *        file.
 *      o EEXIST
 *        path already exists.
 *      o ENOENT
 *        A directory component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int do_mknod(const char *path, int mode, unsigned devid)
{
        dbg(DBG_PRINT, "(GRADING2A)\n");

        if (mode != S_IFCHR && mode != S_IFBLK)
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EINVAL;
        }

        vnode_t *x1 = NULL;
        vnode_t *resultV = NULL;
        size_t len = 0;
        const char *name = NULL;

        int code = dir_namev(path, &len, &name, x1, &resultV);
        if (code < 0)
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return code;
        }
        code = lookup(resultV, name, len, &resultV);
        if (code == 0)
        {
                vput(resultV);
                vput(x1);
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EEXIST;
        }

        KASSERT(NULL != resultV->vn_ops->mknod);
        dbg(DBG_PRINT, "(GRADING2A 3.b)\n");

        code = resultV->vn_ops->mknod(resultV, name, len, mode, devid);
        vput(resultV);
        dbg(DBG_PRINT, "(GRADING2A)\n");
        return code;
}

/* Use dir_namev() to find the vnode of the dir we want to make the new
 * directory in.  Then use lookup() to make sure it doesn't already exist.
 * Finally call the dir's mkdir vn_ops. Return what it returns.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EEXIST
 *        path already exists.
 *      o ENOENT
 *        A directory component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int do_mkdir(const char *path)
{
        dbg(DBG_PRINT, "(GRADING2A)\n");

        size_t namelen;
        const char *name;
        vnode_t *dir_vnode;

        // Extract directory information from the path.
        int result = dir_namev(path, &namelen, &name, NULL, &dir_vnode);
        if (result != 0)
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return result;
        }

        // Early checks for various conditions that could prevent mkdir from proceeding.
        if (namelen == 0 || dir_vnode->vn_ops->mkdir == NULL || namelen > (size_t)STR_MAX)
        {

                result = (namelen == 0) ? 0 : (dir_vnode->vn_ops->mkdir == NULL) ? -ENOTDIR
                                                                                 : -ENAMETOOLONG;
                dbg(DBG_PRINT, "(GRADING2B)\n");

                vput(dir_vnode);
                return result;
        }

        // Try to find if the directory already exists.
        vnode_t *res_vnode_lookup;
        result = lookup(dir_vnode, name, namelen, &res_vnode_lookup);
        if (result == 0)
        { // If the directory already exists, clean up and return -EEXIST.
                dbg(DBG_PRINT, "(GRADING2B)\n");

                vput(res_vnode_lookup);
                vput(dir_vnode);
                return -EEXIST;
        }
        else if (result != -ENOENT)
        { // If lookup failed for a reason other than "not found", clean up and forward the error.
                dbg(DBG_PRINT, "(GRADING2B)\n");

                vput(dir_vnode);
                return result;
        }

        // Confirmed that directory doesn't exist and we can proceed with creation.
        KASSERT(NULL != dir_vnode->vn_ops->mkdir);
        dbg(DBG_PRINT, "(GRADING2A 3.c)\n");
        result = dir_vnode->vn_ops->mkdir(dir_vnode, name, namelen);
        vput(dir_vnode); // Clean up the directory vnode.
        return result;
}

/* Use dir_namev() to find the vnode of the directory containing the dir to be
 * removed. Then call the containing dir's rmdir v_op.  The rmdir v_op will
 * return an error if the dir to be removed does not exist or is not empty, so
 * you don't need to worry about that here. Return the value of the v_op,
 * or an error.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EINVAL
 *        path has "." as its final component.
 *      o ENOTEMPTY
 *        path has ".." as its final component.
 *      o ENOENT
 *        A directory component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int do_rmdir(const char *path)
{
        dbg(DBG_PRINT, "(GRADING2B)\n");

        vnode_t *dir_vnode, *res_vnode;
        size_t namelen;
        const char *name;

        // Early exit if path resolution fails.
        int result = dir_namev(path, &namelen, &name, NULL, &dir_vnode);
        if (result != 0)
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return result;
        }

        // Verify rmdir operation is supported and path does not refer to '.' or '..'.
        // if (dir_vnode->vn_ops->rmdir == NULL)
        if (!(dir_vnode->vn_ops->rmdir))
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");

                result = -ENOTDIR;
        }
        else if ((namelen == 1 && name[0] == '.') || (namelen == 2 && name[0] == '.' && name[1] == '.'))
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");

                result = (namelen == 1) ? -EINVAL : -ENOTEMPTY;
        }
        else
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");

                // Lookup the directory to be removed.
                result = lookup(dir_vnode, name, namelen, &res_vnode);
                if (result == 0)
                {
                        dbg(DBG_PRINT, "(GRADING2B)\n");

                        // Perform the rmdir operation if the directory exists.
                        KASSERT(NULL != dir_vnode->vn_ops->rmdir);
                        dbg(DBG_PRINT, "(GRADING2A 3.d)\n");
                        dbg(DBG_PRINT, "(GRADING2A 3.d)\n");

                        result = dir_vnode->vn_ops->rmdir(dir_vnode, name, namelen);
                        vput(res_vnode); // Release the found vnode regardless of rmdir result.
                }
        }
        dbg(DBG_PRINT, "(GRADING2B)\n");

        vput(dir_vnode); // Cleanup: Release the directory vnode.
        return result;
}

/*
 * Similar to do_rmdir, but for files.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EPERM
 *        path refers to a directory.
 *      o ENOENT
 *        Any component in path does not exist, including the element at the
 *        very end.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int do_unlink(const char *path)
{
        dbg(DBG_PRINT, "(GRADING2B)\n");

        size_t namelen;
        const char *name;
        vnode_t *dirNode, *resultNode;
        int dirNamevResult;

        // Resolve directory and extract name from the given path.
        dirNamevResult = dir_namev(path, &namelen, &name, NULL, &dirNode);
        if (dirNamevResult < 0)
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return dirNamevResult; // Return error from dir_namev if any.
        }

        // Ensure the directory supports unlink operation.
        if (!dirNode->vn_ops->unlink)
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");

                vput(dirNode);   // Release directory vnode reference.
                return -ENOTDIR; // Error if unlink operation is unsupported.
        }

        // Attempt to look up the vnode for the specified name within the directory.
        dirNamevResult = lookup(dirNode, name, namelen, &resultNode);
        if (dirNamevResult < 0)
        {
                vput(dirNode); // Release directory vnode reference on lookup failure.
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return dirNamevResult; // Return lookup error.
        }

        // Prevent unlinking directories through this function.
        if (S_ISDIR(resultNode->vn_mode))
        {
                vput(resultNode); // Release looked up vnode reference.
                vput(dirNode);    // Release directory vnode reference.
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EPERM; // Return error for attempting to unlink a directory.
        }

        // Perform the unlink operation.
        KASSERT(NULL != dirNode->vn_ops->unlink);
        dbg(DBG_PRINT, "(GRADING2A 3.e)\n");
        dbg(DBG_PRINT, "(GRADING2A 3.e)\n");

        dirNamevResult = dirNode->vn_ops->unlink(dirNode, name, namelen);
        vput(resultNode); // Release looked up vnode reference.
        vput(dirNode);    // Release directory vnode reference after operation.
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return dirNamevResult; // Return result of unlink operation.
}

/* To link:
 *      o open_namev(from)
 *      o dir_namev(to)
 *      o call the destination dir's (to) link vn_ops.
 *      o return the result of link, or an error
 *
 * Remember to vput the vnodes returned from open_namev and dir_namev.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EEXIST
 *        to already exists.
 *      o ENOENT
 *        A directory component in from or to does not exist.
 *      o ENOTDIR
 *        A component used as a directory in from or to is not, in fact, a
 *        directory.
 *      o ENAMETOOLONG
 *        A component of from or to was too long.
 *      o EPERM
 *        from is a directory.
 */
int do_link(const char *from, const char *to)
{
        dbg(DBG_PRINT, "(GRADING2B)\n");

        vnode_t *open_vnode, *dir_vnode, *res_vnode;
        size_t namelen;
        const char *name;
        int ret = open_namev(from, 0, &open_vnode, NULL);
        if (ret < 0)
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return ret;
        }

        ret = dir_namev(to, &namelen, &name, NULL, &dir_vnode);
        if (ret < 0)
        {
                vput(open_vnode);
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return ret;
        }

        ret = lookup(dir_vnode, name, namelen, &res_vnode);
        if (ret == 0)
        { // Entry already exists
                vput(dir_vnode);
                vput(open_vnode);
                vput(res_vnode); // Make sure to release the vnode if lookup was successful
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EEXIST;
        }
        else if (ret != -ENOENT)
        { // If the error is not because the entry doesn't exist
                vput(dir_vnode);
                vput(open_vnode);
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return ret; // Return the error from lookup
        }

        // If we get here, it means the entry does not exist and we can proceed with link creation
        ret = dir_vnode->vn_ops->link(open_vnode, dir_vnode, name, namelen);
        vput(dir_vnode);
        vput(open_vnode);
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return ret;
}

/*      o link newname to oldname
 *      o unlink oldname
 *      o return the value of unlink, or an error
 *
 * Note that this does not provide the same behavior as the
 * Linux system call (if unlink fails then two links to the
 * file could exist).
 */
int do_rename(const char *oldname, const char *newname)
{
        dbg(DBG_PRINT, "(GRADING2B)\n");

        int linkResult = do_link(oldname, newname);
        if (linkResult < 0)
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return linkResult;
        }

        int unlinkResult = do_unlink(oldname);
        if (unlinkResult < 0)
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return unlinkResult;
        }

        dbg(DBG_PRINT, "(GRADING2B)\n");
        return unlinkResult;
}

/* Make the named directory the current process's cwd (current working
 * directory).  Don't forget to down the refcount to the old cwd (vput()) and
 * up the refcount to the new cwd (open_namev() or vget()). Return 0 on
 * success.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o ENOENT
 *        path does not exist.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 *      o ENOTDIR
 *        A component of path is not a directory.
 */
int do_chdir(const char *path)
{
        dbg(DBG_PRINT, "(GRADING2B)\n");

        vnode_t *dirNode;
        int openResult;

        // Attempt to resolve the vnode for the given path.
        openResult = open_namev(path, 0, &dirNode, NULL);
        if (openResult < 0)
        {
                dbg(DBG_PRINT, "(GRADING2B)\n"); // Log failure to resolve the path.
                return openResult;               // Return error from opening the path.
        }

        if (dirNode->vn_ops->lookup == NULL)
        {
                vput(dirNode);
                dbg(DBG_PRINT, "(GRADING2B)\n"); // Log the successful change of directory.
                return -ENOTDIR;
        }

        // Update the current process's working directory.
        vput(curproc->p_cwd);     // Release the current directory's vnode.
        curproc->p_cwd = dirNode; // Set the new directory as the current working directory.

        dbg(DBG_PRINT, "(GRADING2B)\n"); // Log the successful change of directory.
        return 0;                        // Return success.
}

/* Call the readdir vn_op on the given fd, filling in the given dirent_t*.
 * If the readdir vn_op is successful, it will return a positive value which
 * is the number of bytes copied to the dirent_t.  You need to increment the
 * file_t's f_pos by this amount.  As always, be aware of refcounts, check
 * the return value of the fget and the virtual function, and be sure the
 * virtual function exists (is not null) before calling it.
 *
 * Return either 0 or sizeof(dirent_t), or -errno.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        Invalid file descriptor fd.
 *      o ENOTDIR
 *        File descriptor does not refer to a directory.
 */
int do_getdent(int fd, struct dirent *dirp)
{
        dbg(DBG_PRINT, "(GRADING2B)\n");

        // Validate file descriptor.
        if (fd < 0 || fd >= NFILES || !curproc->p_files[fd])
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF;
        }

        // Retrieve and validate the file associated with the file descriptor.
        file_t *file = fget(fd);

        // Ensure the file is a directory and supports readdir operation.
        if (!S_ISDIR(file->f_vnode->vn_mode) || !file->f_vnode->vn_ops->readdir)
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                fput(file);                                                       // Release the file.
                return !S_ISDIR(file->f_vnode->vn_mode) ? -ENOTDIR : -EOPNOTSUPP; // Operation not supported or not a directory.
        }

        // Perform the directory read operation.
        int bufferByte = file->f_vnode->vn_ops->readdir(file->f_vnode, file->f_pos, dirp);

        // If bytes were read, update the file position.
        if (bufferByte > 0)
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");

                file->f_pos += bufferByte;
        }

        // Release the file and return the result.
        fput(file);
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return bufferByte > 0 ? sizeof(*dirp) : 0;
}

/*
 * Modify f_pos according to offset and whence.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not an open file descriptor.
 *      o EINVAL
 *        whence is not one of SEEK_SET, SEEK_CUR, SEEK_END; or the resulting
 *        file offset would be negative.
 */
int do_lseek(int fd, int offset, int whence)
{
        dbg(DBG_PRINT, "(GRADING2B)\n");

        // Validate file descriptor.
        if (fd < 0 || fd >= NFILES || !curproc->p_files[fd])
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF;
        }

        // Acquire the file object.
        file_t *file = fget(fd);
        if (!file)
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EBADF;
        }

        // Calculate the new position based on 'whence' and 'offset'.
        off_t updatedPointer;
        dbg(DBG_PRINT, "(GRADING2B)\n");

        switch (whence)
        {
        case SEEK_SET:
                dbg(DBG_PRINT, "(GRADING2B)\n");

                updatedPointer = offset;
                break;
        case SEEK_CUR:
                dbg(DBG_PRINT, "(GRADING2B)\n");

                updatedPointer = file->f_pos + offset;
                break;
        case SEEK_END:
                dbg(DBG_PRINT, "(GRADING2B)\n");

                updatedPointer = file->f_vnode->vn_len + offset;
                break;
        }
        dbg(DBG_PRINT, "(GRADING2B)\n");

        // Validate the new position.
        if (updatedPointer < 0 || (whence == SEEK_END && offset < 0 && -offset > file->f_vnode->vn_len))
        {
                fput(file);
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EINVAL;
        }

        // Update the file position.
        file->f_pos = updatedPointer;
        fput(file); // Release the file object.

        dbg(DBG_PRINT, "(GRADING2B)\n");
        return file->f_pos; // Return the new position.
}

/*
 * Find the vnode associated with the path, and call the stat() vnode operation.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o ENOENT
 *        A component of path does not exist.
 *      o ENOTDIR
 *        A component of the path prefix of path is not a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 *      o EINVAL
 *        path is an empty string.
 */
int do_stat(const char *path, struct stat *buf)
{
        dbg(DBG_PRINT, "(GRADING2B)\n");

        // Validate the path is not empty.
        if (strlen(path) == 0)
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EINVAL; // Empty path is invalid.
        }

        // Attempt to resolve the vnode from the given path.
        vnode_t *target_vnode = NULL;
        int openResult = open_namev(path, 0, &target_vnode, NULL);
        if (openResult < 0)
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return openResult; // Return error encountered during vnode resolution.
        }

        // Perform the stat operation on the resolved vnode.
        KASSERT(NULL != target_vnode->vn_ops->stat);
        dbg(DBG_PRINT, "(GRADING2A 3.f)\n");
        dbg(DBG_PRINT, "(GRADING2A 3.f)\n");

        openResult = target_vnode->vn_ops->stat(target_vnode, buf);

        // Cleanup: Release the vnode after use.
        vput(target_vnode);
        dbg(DBG_PRINT, "(GRADING2B)\n");
        return openResult; // Return the result of the stat operation.
}

#ifdef __MOUNTING__
/*
 * Implementing this function is not required and strongly discouraged unless
 * you are absolutely sure your Weenix is perfect.
 *
 * This is the syscall entry point into vfs for mounting. You will need to
 * create the fs_t struct and populate its fs_dev and fs_type fields before
 * calling vfs's mountfunc(). mountfunc() will use the fields you populated
 * in order to determine which underlying filesystem's mount function should
 * be run, then it will finish setting up the fs_t struct. At this point you
 * have a fully functioning file system, however it is not mounted on the
 * virtual file system, you will need to call vfs_mount to do this.
 *
 * There are lots of things which can go wrong here. Make sure you have good
 * error handling. Remember the fs_dev and fs_type buffers have limited size
 * so you should not write arbitrary length strings to them.
 */
int do_mount(const char *source, const char *target, const char *type)
{
        NOT_YET_IMPLEMENTED("MOUNTING: do_mount");
        return -EINVAL;
}

/*
 * Implementing this function is not required and strongly discouraged unless
 * you are absolutley sure your Weenix is perfect.
 *
 * This function delegates all of the real work to vfs_umount. You should not worry
 * about freeing the fs_t struct here, that is done in vfs_umount. All this function
 * does is figure out which file system to pass to vfs_umount and do good error
 * checking.
 */
int do_umount(const char *target)
{
        NOT_YET_IMPLEMENTED("MOUNTING: do_umount");
        return -EINVAL;
}
#endif
