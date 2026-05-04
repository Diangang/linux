/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LINUX_VFS_DEBUG_H
#define LINUX_VFS_DEBUG_H 1

#include <linux/bug.h>

struct inode;

#define VFS_BUG_ON(cond) BUILD_BUG_ON_INVALID(cond)
#define VFS_WARN_ON(cond) BUILD_BUG_ON_INVALID(cond)
#define VFS_WARN_ON_ONCE(cond) BUILD_BUG_ON_INVALID(cond)
#define VFS_WARN_ONCE(cond, format...) BUILD_BUG_ON_INVALID(cond)
#define VFS_WARN(cond, format...) BUILD_BUG_ON_INVALID(cond)

#define VFS_BUG_ON_INODE(cond, inode) VFS_BUG_ON(cond)
#define VFS_WARN_ON_INODE(cond, inode)  BUILD_BUG_ON_INVALID(cond)

#endif
