/* Copyright (c) Piotr Durlej
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <xenus/process.h>
#include <xenus/syscall.h>
#include <xenus/umem.h>
#include <xenus/fs.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

static int sticky(struct inode *dir, struct inode *ino)
{
	if (!(dir->d.mode & 01000))
		return 0;
	if (!curr->euid)
		return 0;
	if (ino->d.uid == curr->euid)
		return 0;
	return EPERM;
}

char *basename(char *path)
{
	char *s;
	
	s = strrchr(path, '/');
	if (!s)
		return path;
	else
		return s + 1;
}

int do_mknod(char *path, mode_t mode, dev_t dev)
{
	struct inode *dir = NULL;
	struct inode *ino = NULL;
	int err;
	
	if (curr->euid && !S_ISREG(mode) && !S_ISFIFO(mode))
		return EPERM;
	
	err = dir_traverse(path, 1, &dir);
	if (err)
		goto fail;
	
	err = alloc_inode(dir->sb, &ino);
	if (err)
		goto fail;
	
	err = dir_creat(dir, basename(path), ino->ino);
	if (err)
		goto fail;
	
	ino->d.mode	= mode & ~curr->umask;
	ino->d.rdev	= dev;
	ino->d.nlink	= 1;
	ino->dirty	= 2;
	
fail:
	inode_put(dir);
	inode_put(ino);
	return 0;
}

int do_unlink(char *name, int spec)
{
	struct inode *dir = NULL;
	struct inode *ino = NULL;
	struct dirpos dp;
	int err;
	
	dp.buf = NULL;
	
	err = dir_traverse(name, 1, &dir);
	if (err)
		return err;
	
	err = dir_search(dir, basename(name), &dp);
	if (err)
	{
		inode_put(dir);
		return err;
	}
	
	err = inode_chkperm(dir, W_OK);
	if (err)
		goto fail;
	
	err = inode_get(dir->sb, dp.de->d_ino, &ino);
	if (err)
		goto fail;
	
	if (S_ISDIR(ino->d.mode))
	{
		if (!spec)
		{
			err = EISDIR;
			goto fail;
		}
		dir->d.nlink--;
		dir->dirty = 2;
	}
	
	if (ino->sb != dir->sb)
	{
		err = EXDEV;
		goto fail;
	}
	
	err = sticky(dir, ino);
	if (err)
		goto fail;
	
	if (S_ISDIR(ino->d.mode))
		ino->d.mode = S_IFREG;
	
	ino->d.nlink--;
	ino->dirty = 2;
	memset(dp.de, 0, sizeof(struct dirent));
	dp.buf->dirty = 1;
fail:
	if (dp.buf)
		dirpos_put(dp);
	inode_put(dir);
	inode_put(ino);
	return err;
}

int do_mkdir(char *path, mode_t mode)
{
	struct inode *dir = NULL;
	struct inode *ino = NULL;
	int err;
	
	err = dir_traverse(path, 1, &dir);
	if (err)
		return err;
	
	if (dir->d.nlink >= NLINK_MAX)
	{
		err = EMLINK;
		goto fail;
	}
	
	err = alloc_inode(dir->sb, &ino);
	if (err)
		goto fail;
	
	ino->d.mode = S_IFDIR | (mode & 07777 & ~curr->umask);
	ino->dirty  = 2;
	
	err = dir_creat(ino, ".", ino->ino);
	if (err)
		goto fail;
	
	err = dir_creat(ino, "..", dir->ino);
	if (err)
		goto fail;
	
	err = dir_creat(dir, basename(path), ino->ino);
	if (err)
		goto fail;
	
	dir->d.nlink++;
	dir->dirty = 1;
	
	ino->d.nlink = 1;
	ino->dirty   = 2;
fail:
	inode_put(ino);
	inode_put(dir);
	return err;
}

int do_rmdir(char *path)
{
	struct inode *ino;
	int err;
	
	if (!strcmp(basename(path), "."))
		return EPERM;
	
	err = dir_traverse(path, 0, &ino);
	if (err)
		return err;
	
	err = dir_isempty(ino);
	if (err)
	{
		inode_put(ino);
		return err;
	}
	inode_put(ino);
	
	return do_unlink(path, 1);
}

int do_link(char *name1, char *name2)
{
	struct inode *ino = NULL;
	struct inode *dir = NULL;
	int err;
	
	err = dir_traverse(name1, 0, &ino);
	if (err)
		return err;
	
	if (ino->d.nlink >= NLINK_MAX)
	{
		err = EMLINK;
		goto fail;
	}
	
	if (S_ISDIR(ino->d.mode))
	{
		err = EISDIR;
		goto fail;
	}
	
	err = dir_traverse(name2, 1, &dir);
	if (err)
		goto fail;
	
	if (ino->sb != dir->sb)
	{
		err = EXDEV;
		goto fail;
	}
	
	err = sticky(dir, ino);
	if (err)
		goto fail;
	
	err = dir_creat(dir, basename(name2), ino->ino);
	if (!err)
	{
		ino->d.nlink++;
		ino->dirty = 2;
	}
fail:
	inode_put(ino);
	inode_put(dir);
	return err;
}

int do_rename(char *name1, char *name2)
{
	struct inode *dir1 = NULL;
	struct inode *dir2 = NULL;
	struct inode *ino1 = NULL;
	struct inode *ino2 = NULL;
	struct dirpos dp1;
	struct dirpos dp2;
	int err;
	
	dp1.buf = NULL;
	dp2.buf = NULL;
	
	if (!strcmp(basename(name1), "."))
		return EINVAL;
	if (!strcmp(basename(name1), ".."))
		return EINVAL;
	if (!strcmp(basename(name2), "."))
		return EINVAL;
	if (!strcmp(basename(name2), ".."))
		return EINVAL;
	
	err = dir_traverse(name1, 1, &dir1);
	if (err)
		goto ret;
	err = inode_chkperm(dir1, W_OK);
	if (err)
		goto ret;
	err = dir_traverse(name2, 1, &dir2);
	if (err)
		goto ret;
	err = inode_chkperm(dir2, W_OK);
	if (err)
		goto ret;
	if (dir1->sb != dir2->sb)
	{
		err = EXDEV;
		goto ret;
	}
	err = dir_search(dir1, basename(name1), &dp1);
	if (err)
		goto ret;
	err = inode_get(dir1->sb, dp1.de->d_ino, &ino1);
	if (err)
		goto ret;
	if (dir1->sb != ino1->sb)
	{
		err = EXDEV;
		goto ret;
	}
	
	err = sticky(dir1, ino1);
	if (err)
		goto ret;
	
	err = dir_creat(dir2, basename(name2), ino1->ino);
	if (err == EEXIST)
	{
		err = dir_search(dir2, basename(name2), &dp2);
		if (err)
			goto ret;
		err = inode_get(dir2->sb, dp2.de->d_ino, &ino2);
		if (err)
			goto ret;
		if (ino2->ino == ino1->ino && ino2->sb == ino1->sb)
		{
			err = 0;
			goto ret;
		}
		if (S_ISDIR(ino2->d.mode))
		{
			err = EISDIR;
			goto ret;
		}
		
		err = sticky(dir2, ino2);
		if (err)
			goto ret;
		
		ino2->d.nlink--;
		ino2->dirty = 2;
		dp2.de->d_ino = dp1.de->d_ino;
		strncpy(dp2.de->d_name, basename(name2), NAME_MAX);
		dp2.buf->dirty = 1;
	}
	
	if (S_ISDIR(ino1->d.mode))
	        err = dir_reparent(ino1, dir2->ino);
	
	memset(dp1.de, 0, sizeof(*dp1.de));
	dp1.buf->dirty = 1;
	
ret:
	inode_put(dir1);
	inode_put(dir2);
	inode_put(ino1);
	inode_put(ino2);
	if (dp1.buf)
		dirpos_put(dp1);
	if (dp2.buf)
		dirpos_put(dp2);
	return err;
}

int sys_mknod(char *path, mode_t mode, dev_t dev)
{
	char lpath[PATH_MAX];
	int err;
	
	err = fustr(lpath, path, PATH_MAX);
	if (err)
		goto fail;
	
	err = do_mknod(lpath, mode, dev);
	if (err)
		goto fail;
	return 0;
fail:
	uerr(err);
	return -1;
}

int sys_mkdir(char *path, mode_t mode)
{
	char lpath[PATH_MAX];
	int err;
	
	err = fustr(lpath, path, PATH_MAX);
	if (err)
		goto fail;
	
	err = do_mkdir(lpath, mode);
	if (err)
		goto fail;
	return 0;
fail:
	uerr(err);
	return -1;
}

int sys_rmdir(char *path)
{
	char lpath[PATH_MAX];
	int err;
	
	err = fustr(lpath, path, PATH_MAX);
	if (err)
		goto fail;
	
	err = do_rmdir(lpath);
	if (err)
		goto fail;
	return 0;
fail:
	uerr(err);
	return -1;
}

int sys_unlink(char *name)
{
	char lname[PATH_MAX];
	int err;
	
	err = fustr(lname, name, PATH_MAX);
	if (err)
		goto fail;
	
	err = do_unlink(lname, 0);
	if (err)
		goto fail;
	return 0;
fail:
	uerr(err);
	return -1;
}

int sys_link(char *name1, char *name2)
{
	char lname1[PATH_MAX];
	char lname2[PATH_MAX];
	int err;
	
	err = fustr(lname1, name1, PATH_MAX);
	if (err)
		goto fail;
	
	err = fustr(lname2, name2, PATH_MAX);
	if (err)
		goto fail;
	
	err = do_link(lname1, lname2);
	if (err)
		goto fail;
	return 0;
fail:
	uerr(err);
	return -1;
}

int sys_rename(char *name1, char *name2)
{
	char lname1[PATH_MAX];
	char lname2[PATH_MAX];
	int err;
	
	err = fustr(lname1, name1, PATH_MAX);
	if (err)
		goto fail;
	
	err = fustr(lname2, name2, PATH_MAX);
	if (err)
		goto fail;
	
	err = do_rename(lname1, lname2);
	if (err)
		goto fail;
	return 0;
fail:
	uerr(err);
	return -1;
}

int sys_umask(mode_t mask)
{
	mode_t prev = curr->umask;
	
	curr->umask = mask & 0777;
	return prev;
}
