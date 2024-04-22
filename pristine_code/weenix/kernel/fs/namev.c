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
#include "globals.h"
#include "types.h"
#include "errno.h"

#include "util/string.h"
#include "util/printf.h"
#include "util/debug.h"

#include "fs/dirent.h"
#include "fs/fcntl.h"
#include "fs/stat.h"
#include "fs/vfs.h"
#include "fs/vnode.h"

/* This takes a base 'dir', a 'name', its 'len', and a result vnode.
 * Most of the work should be done by the vnode's implementation
 * specific lookup() function.
 *
 * If dir has no lookup(), return -ENOTDIR.
 *
 * Note: returns with the vnode refcount on *result incremented.
 */
int lookup(vnode_t *dir, const char *name, size_t len, vnode_t **result)
{
        KASSERT(NULL != dir);
        KASSERT(NULL != name);
        KASSERT(NULL != result);
        dbg(DBG_PRINT, "(GRADING2A 2.a)\n");
        dbg(DBG_PRINT, "(GRADING2A)\n");

        if (len > STR_MAX)
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -ENAMETOOLONG;
        }

        dbg(DBG_PRINT, "(GRADING2A)\n");

        if (!dir->vn_ops->lookup)
        {
                dbg(DBG_PRINT, "(GRADING2A)\n");
                return -ENOTDIR;
        }

        dbg(DBG_PRINT, "(GRADING2A)\n");
        int ret = dir->vn_ops->lookup(dir, name, len, result);

        return ret;
}

/* When successful this function returns data in the following "out"-arguments:
 *  o res_vnode: the vnode of the parent directory of "name"
 *  o name: the `basename' (the element of the pathname)
 *  o namelen: the length of the basename
 *
 * For example: dir_namev("/s5fs/bin/ls", &namelen, &name, NULL,
 * &res_vnode) would put 2 in namelen, "ls" in name, and a pointer to the
 * vnode corresponding to "/s5fs/bin" in res_vnode.
 *
 * The "base" argument defines where we start resolving the path from:
 * A base value of NULL means to use the process's current working directory,
 * curproc->p_cwd.  If pathname[0] == '/', ignore base and start with
 * vfs_root_vn.  dir_namev() should call lookup() to take care of resolving each
 * piece of the pathname.
 *
 * Note: A successful call to this causes vnode refcount on *res_vnode to
 * be incremented.
 */
void nextSlash(const char *pathname, int *fast, int *resVNodeDirIndexLimit)
{
        while (*fast < *resVNodeDirIndexLimit)
        {

                if ('/' == pathname[*fast])
                {
                        dbg(DBG_PRINT, "(GRADING2B)\n");
                        break;
                }

                (*fast)++;
                dbg(DBG_PRINT, "(GRADING2A)\n");
        }
        dbg(DBG_PRINT, "(GRADING2A)\n");
}

void nextNonSlash(const char *pathname, int *fast, int *resVNodeDirIndexLimit)
{
        while (*fast < *resVNodeDirIndexLimit)
        {
                if ('/' != pathname[*fast])
                {
                        dbg(DBG_PRINT, "(GRADING2A)\n");
                        break;
                }
                (*fast)++;
                dbg(DBG_PRINT, "(GRADING2A)\n");
        }
        dbg(DBG_PRINT, "(GRADING2A)\n");
}

vnode_t *resolveBaseVNode(const char *pathname, vnode_t *base)
{
        // If the pathname starts with '/', use the root vnode
        if (pathname[0] == '/')
        {
                dbg(DBG_PRINT, "(GRADING2A)\n");
                return vfs_root_vn;
        }
        // If base is NULL, use the current process's working directory
        else if (base == NULL)
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return curproc->p_cwd;
        }
        dbg(DBG_PRINT, "(GRADING2B)\n");
        // Otherwise, use the provided base vnode
        return base;
}
int finalizeResultVNode(vnode_t **res_vnode, vnode_t *curVNode, vnode_t *curBase)
{
        // Ensure the result vnode pointer is valid.
        if (!res_vnode)
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EINVAL; // Invalid argument.
        }

        // If we have a current vnode to consider.
        if (curVNode)
        {
                // Check if the current vnode is a valid directory.
                if (!curBase->vn_ops->lookup)
                {
                        vput(curVNode); // Decrement the reference count as it's being released.
                        dbg(DBG_PRINT, "(GRADING2B)\n");
                        return -ENOTDIR; // The current vnode is not a directory.
                }

                *res_vnode = curVNode; // Assign the current vnode as the result vnode.
                dbg(DBG_PRINT, "(GRADING2A)\n");
        }
        else
        {
                // If there's no current vnode, fall back to the base vnode.
                *res_vnode = curBase;
                dbg(DBG_PRINT, "(GRADING2A)\n");

                if (curBase)
                {
                        dbg(DBG_PRINT, "(GRADING2A)\n");
                        vref(*res_vnode); // Increment the reference count of the base vnode.
                }
                else
                {
                        dbg(DBG_PRINT, "(GRADING2B)\n");
                        return -EINVAL; // No valid vnode to return.
                }
        }
        dbg(DBG_PRINT, "(GRADING2A)\n");
        return 0; // Indicate success.
}

int dir_namev(const char *pathname, size_t *namelen, const char **name,
              vnode_t *base, vnode_t **res_vnode)
{

        KASSERT(NULL != pathname);
        KASSERT(NULL != namelen);
        KASSERT(NULL != name);
        KASSERT(NULL != res_vnode);
        dbg(DBG_PRINT, "(GRADING2A 2.b)\n");
        vnode_t *curBase = base;

        curBase = resolveBaseVNode(pathname, base);

        KASSERT(NULL != curBase);
        dbg(DBG_PRINT, "(GRADING2A 2.b)\n");

        int resVNodeDirIndexLimit = strlen(pathname) - 1;
        int last_slash_len = 0;

        for (; resVNodeDirIndexLimit >= 0 && pathname[resVNodeDirIndexLimit] == '/'; resVNodeDirIndexLimit--)
        {
                last_slash_len++;
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }
        dbg(DBG_PRINT, "(GRADING2A)\n");

        for (; resVNodeDirIndexLimit >= 0; resVNodeDirIndexLimit--)
        {
                if ('/' == pathname[resVNodeDirIndexLimit])
                {
                        dbg(DBG_PRINT, "(GRADING2A)\n");
                        break;
                }
                dbg(DBG_PRINT, "(GRADING2A)\n");
        }
        dbg(DBG_PRINT, "(GRADING2A)\n");

        *namelen = strlen(pathname) - last_slash_len - (resVNodeDirIndexLimit + 1);

        // Check if there's a valid segment to consider as the name
        if (*namelen > 0)
        {
                // Validate against the maximum allowed name length
                if (*namelen > NAME_LEN)
                {
                        dbg(DBG_PRINT, "(GRADING2B)\n");
                        return -ENAMETOOLONG; // Name length exceeds the allowed limit
                }

                // Determine the starting point of the name based on the presence of slashes
                *name = (resVNodeDirIndexLimit >= 0) ? &pathname[resVNodeDirIndexLimit + 1] : pathname;
                dbg(DBG_PRINT, "(GRADING2A)\n");
        }
        else
        {
                // If there's no valid segment, set name to NULL
                *name = NULL;
                dbg(DBG_PRINT, "(GRADING2B)\n");
        }
        dbg(DBG_PRINT, "(GRADING2A)\n");

        int slow = 0;
        int fast = 0;
        vnode_t *curVNode = NULL;
        vnode_t *prevVNode = NULL;

        nextNonSlash(pathname, &fast, &resVNodeDirIndexLimit);
        dbg(DBG_PRINT, "(GRADING2A)\n");

        while (fast < resVNodeDirIndexLimit)
        {

                slow = fast;

                nextSlash(pathname, &fast, &resVNodeDirIndexLimit);
                dbg(DBG_PRINT, "(GRADING2A)\n");

                if (fast - slow > NAME_LEN)
                {
                        vput(curVNode); // Release the current vnode due to error
                        dbg(DBG_PRINT, "(GRADING2B)\n");
                        return -ENAMETOOLONG; // Return the corresponding error code
                }
                dbg(DBG_PRINT, "(GRADING2A)\n");

                // Perform lookup for the current path component
                int lookupResult = lookup(curBase, &pathname[slow], fast - slow, &curVNode);
                dbg(DBG_PRINT, "(GRADING2A)\n");

                if (lookupResult < 0)
                {
                        if (prevVNode)
                        {
                                vput(prevVNode); // Ensure to release the previous vnode if lookup failed
                                dbg(DBG_PRINT, "(GRADING2B)\n");
                        }
                        dbg(DBG_PRINT, "(GRADING2B)\n");
                        return lookupResult; // Propagate the lookup error
                }
                dbg(DBG_PRINT, "(GRADING2A)\n");

                // Release the reference to the previous vnode, if any, as we move forward
                if (prevVNode)
                {
                        vput(prevVNode);
                        dbg(DBG_PRINT, "(GRADING2B)\n");
                }
                dbg(DBG_PRINT, "(GRADING2A)\n");

                // Update tracking pointers for the next iteration
                prevVNode = curVNode;
                curBase = curVNode;

                // skip slash
                nextNonSlash(pathname, &fast, &resVNodeDirIndexLimit);
                dbg(DBG_PRINT, "(GRADING2A)\n");
        }

        int result = finalizeResultVNode(res_vnode, curVNode, curBase);
        dbg(DBG_PRINT, "(GRADING2A)\n");
        if (result != 0)
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return result; // Handle any errors that occurred during vnode finalization
        }

        dbg(DBG_PRINT, "(GRADING2A)\n");
        return 0; // Indicate success
}

/* This returns in res_vnode the vnode requested by the other parameters.
 * It makes use of dir_namev and lookup to find the specified vnode (if it
 * exists).  flag is right out of the parameters to open(2); see
 * <weenix/fcntl.h>.  If the O_CREAT flag is specified and the file does
 * not exist, call create() in the parent directory vnode. However, if the
 * parent directory itself does not exist, this function should fail - in all
 * cases, no files or directories other than the one at the very end of the path
 * should be created.
 *
 * Note: Increments vnode refcount on *res_vnode.
 */
/*
// Mine
int open_namev(const char *pathname, int flag, vnode_t **res_vnode, vnode_t *base)
{

        vnode_t *nodev = NULL;
        size_t len = 0;
        const char *name = NULL;

        // Resolve the directory vnode and the final component name of the path
        int dret = dir_namev(pathname, &len, &name, base, &nodev);

        if (dret != 0)
        {
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return dret; // Return early if path resolution fails
        }
        dbg(DBG_PRINT, "(GRADING2A)\n");

        if (len == 0)
        {
                // set res_vnode to parent directory of res_vnode
                *res_vnode = nodev;
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return 0;
        }
        dbg(DBG_PRINT, "(GRADING2A)\n");

        // Attempt to lookup the name within the resolved directory
        int lret = lookup(nodev, name, len, res_vnode);

        // If the name doesn't exist and creation is requested, attempt to create the file
        if (lret == -ENOENT && (flag & O_CREAT))
        {
                KASSERT(NULL != nodev->vn_ops->create); // Ensure creation is supported
                dbg(DBG_PRINT, "(GRADING2A 2.c)\n");
                dbg(DBG_PRINT, "(GRADING2A 2.c)\n");

                int res_create = nodev->vn_ops->create(nodev, name, len, res_vnode);
                vput(nodev); // Release the directory vnode as it's no longer needed
                dbg(DBG_PRINT, "(GRADING2B)\n");

                return res_create;
        }
        dbg(DBG_PRINT, "(GRADING2A)\n");

        // check if pathname requires a directory
        if (pathname[strlen(pathname) - 1] == '/' && !S_ISDIR((*res_vnode)->vn_mode))
        {
                vput((*res_vnode));
                vput(nodev);
                dbg(DBG_PRINT, "(GRADING2B)\n");
                return -ENOTDIR;
        }

        vput(nodev); // Release the directory vnode as it's no longer needed
        dbg(DBG_PRINT, "(GRADING2A)\n");
        return lret; // Return the result of the lookup or creation attempt
}
*/

// Ridheesh
int open_namev(const char *pathname, int flag, vnode_t **res_vnode, vnode_t *base)
{
        // NOT_YET_IMPLEMENTED("VFS: open_namev");

        size_t namelen = 0;
        const char *name = NULL;
        vnode_t *resVNodeDir = NULL;

        // resolve pathname
        int code = dir_namev(pathname, &namelen, &name, base, &resVNodeDir);
        // if code < 0, parent directory does not exist
        if (code < 0)
        {
                return code;
        }

        // if namelen == 0, pathname has no last file
        // but pathname exists, so there is no need to create any file
        if (namelen == 0)
        {
                // set res_vnode to parent directory of res_vnode
                *res_vnode = resVNodeDir;
                return 0;
        }
        // if pathname has last file
        // lookup last file of pathname
        code = lookup(resVNodeDir, name, namelen, res_vnode);

        // if code < 0, last file of pathname does not exist
        if (code < 0)
        {
                // if O_CREAT flag is specified
                if (flag & O_CREAT)
                {
                        // it is time to create last file
                        // check create() of res_vnode parent directory
                        KASSERT(NULL != resVNodeDir->vn_ops->create);

                        // create last file
                        code = resVNodeDir->vn_ops->create(resVNodeDir, name, namelen, res_vnode);
                        // create() has set res_vnode VNode and increased res_vnode vn_refcount
                        vput(resVNodeDir);

                        return code;
                }
                // if O_CREAT flag is not specified
                vput(resVNodeDir);

                return code;
        }
        // if code == 0, last file of pathname exists

        // check if pathname requires a directory
        if (pathname[strlen(pathname) - 1] == '/' && !S_ISDIR((*res_vnode)->vn_mode))
        {
                vput((*res_vnode));
                vput(resVNodeDir);
                return -ENOTDIR;
        }
        // lookup() has set res_vnode VNode and increased res_vnode vn_refcount
        vput(resVNodeDir);

        return 0;
}

#ifdef __GETCWD__
/* Finds the name of 'entry' in the directory 'dir'. The name is writen
 * to the given buffer. On success 0 is returned. If 'dir' does not
 * contain 'entry' then -ENOENT is returned. If the given buffer cannot
 * hold the result then it is filled with as many characters as possible
 * and a null terminator, -ERANGE is returned.
 *
 * Files can be uniquely identified within a file system by their
 * inode numbers. */
int lookup_name(vnode_t *dir, vnode_t *entry, char *buf, size_t size)
{
        NOT_YET_IMPLEMENTED("GETCWD: lookup_name");
        return -ENOENT;
}

/* Used to find the absolute path of the directory 'dir'. Since
 * directories cannot have more than one link there is always
 * a unique solution. The path is writen to the given buffer.
 * On success 0 is returned. On error this function returns a
 * negative error code. See the man page for getcwd(3) for
 * possible errors. Even if an error code is returned the buffer
 * will be filled with a valid string which has some partial
 * information about the wanted path. */
ssize_t
lookup_dirpath(vnode_t *dir, char *buf, size_t osize)
{
        NOT_YET_IMPLEMENTED("GETCWD: lookup_dirpath");

        return -ENOENT;
}
#endif /* __GETCWD__ */
