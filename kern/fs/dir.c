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
#include <xenus/fs.h>
#include <sys/stat.h>
#include <limits.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>

int dir_search(struct inode *dir, char *name, struct dirpos *dp)
{
	struct blk_buffer *b;
	struct dirent *de;
	blk_t n;
	int err;
	int i;
	
	err = inode_chkperm(dir, R_OK | X_OK);
	if (err)
		return err;
	if (!S_ISDIR(dir->d.mode))
		return ENOTDIR;
	
	for (n = 0; ; n++)
	{
		err = bmget(dir, n, &b);
		if (err)
			return err;
		if (!b)
			return ENOENT;
		for (i = 0; i < BLK_SIZE / sizeof(struct dirent); i++)
		{
			de = (struct dirent *)b->data + i;
			
			if (de->d_ino && !strncmp(de->d_name, name, NAME_MAX))
			{
				dp->buf	= b;
				dp->de	= de;
				return 0;
			}
		}
		blk_put(b);
	}
}

int dir_search_i(struct inode *dir, char *name, struct inode **ino)
{
	struct dirpos dp;
	int err;
	
	if (!*name)
	{
		*ino = dir;
		dir->refcnt++;
		return 0;
	}
	
	if (!strcmp(name, ".."))
	{
		if (dir == curr->root)
		{
			*ino = dir;
			dir->refcnt++;
			return 0;
		}
		if (dir->mnt_dir)
			dir = dir->mnt_dir;
	}
	
	err = dir_search(dir, name, &dp);
	if (err)
		return err;
	
	err = inode_get(dir->sb, dp.de->d_ino, ino);
	dirpos_put(dp);
	return err;
}

int dir_creat(struct inode *dir, char *name, ino_t ino)
{
	struct blk_buffer *b;
	struct dirent *de;
	blk_t fn;
	blk_t n;
	int err;
	int fi = -1;
	int i;
	
	if (strlen(name) > NAME_MAX)
		return ENAMETOOLONG;
	if (!*name)
		return EINVAL;
	
	if (!S_ISDIR(dir->d.mode))
		return ENOTDIR;
	
	for (n = 0; ; n++)
	{
		err = bmget(dir, n, &b);
		if (err)
			return err;
		if (!b)
			break;
		for (i = 0; i < BLK_SIZE / sizeof(struct dirent); i++)
		{
			de = (struct dirent *)b->data + i;
			
			if (de->d_ino && !strncmp(de->d_name, name, NAME_MAX))
			{
				blk_put(b);
				return EEXIST;
			}
			
			if (!de->d_ino && fi == -1)
			{
				fi = i;
				fn = n;
			}
		}
		blk_put(b);
	}
	
	if (fi == -1)
	{
		fn = n;
		fi = 0;
	}
	
	if (fn * BLK_SIZE + (fi + 1) * sizeof(struct dirent) > dir->d.size)
	{
		dir->d.size = fn * BLK_SIZE + (fi + 1) * sizeof(struct dirent);
		dir->dirty  = 2;
	}
	
	err = inode_chkperm(dir, W_OK);
	if (err)
		return err;
	
	err = bmget_alloc(dir, fn, &b);
	if (err)
		return err;
	de = (struct dirent *)b->data + fi;
	de->d_ino = ino;
	strncpy(de->d_name, name, NAME_MAX);
	b->dirty = 1;
	blk_put(b);
	return 0;
}

int dir_traverse(char *path, int parent, struct inode **ino)
{
	char name[NAME_MAX + 1];
	struct inode *c;
	struct inode *n;
	char *sl;
	int err;
	int l;
	
	if (*path == '/')
	{
		c = curr->root;
		path++;
	}
	else
		c = curr->cwd;
	
	c->refcnt++;
	
	do
	{
		sl = strchr(path, '/');
		if (sl)
			l = sl - path;
		else
		{
			l = strlen(path);
			if (parent)
				break;
		}
		
		if (l > NAME_MAX)
			return ENAMETOOLONG;
		
		memcpy(name, path, l);
		name[l] = 0;
		
		err = dir_search_i(c, name, &n);
		inode_put(c);
		if (err)
			return err;
		c = n;
		path += l;
		path++;
	} while (sl);
	
	*ino = c;
	return 0;
}

int dir_isempty(struct inode *dir)
{
	struct blk_buffer *b;
	struct dirent *de;
	blk_t n;
	int err;
	int i;
	
	if (!S_ISDIR(dir->d.mode))
		return ENOTDIR;
	
	for (n = 0; ; n++)
	{
		err = bmget(dir, n, &b);
		if (err)
			return err;
		if (!b)
			return 0;
		for (i = 0; i < BLK_SIZE / sizeof(struct dirent); i++)
		{
			de = (struct dirent *)b->data + i;
			
			if (!strncmp(de->d_name, ".", NAME_MAX))
				continue;
			if (!strncmp(de->d_name, "..", NAME_MAX))
				continue;
			if (de->d_ino)
			{
				blk_put(b);
				return ENOTEMPTY;
			}
		}
		blk_put(b);
	}
}

int dir_reparent(struct inode *dir, ino_t ino)
{
	struct blk_buffer *b;
	struct dirent *de;
	blk_t n;
	int err;
	int i;
	
	err = inode_chkperm(dir, W_OK | X_OK);
	if (err)
		return err;
	if (!S_ISDIR(dir->d.mode))
		return ENOTDIR;
	
	err = bmget(dir, 0, &b);
	if (err)
		return err;
	if (!b)
		return EINVAL;
	
	for (i = 0; i < BLK_SIZE / sizeof(struct dirent); i++)
	{
		de = (struct dirent *)b->data + i;
		
		if (!strncmp(de->d_name, "..", NAME_MAX))
		{
			de->d_ino = ino;
			b->valid = 1;
			b->dirty = 1;
			blk_put(b);
			return 0;
		}
	}
	blk_put(b);
	return EINVAL;
}
