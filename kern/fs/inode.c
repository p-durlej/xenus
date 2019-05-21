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
#include <xenus/printf.h>
#include <xenus/panic.h>
#include <xenus/page.h>
#include <xenus/fs.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

struct inode inode[MAXINODES];

int inode_read(struct inode *ino)
{
	struct blk_buffer *b;
	int err;
	
	err = blk_read(ino->sb->dev, ino->ino, &b);
	if (err)
		return err;
	
	memcpy(&ino->d, b->data, sizeof(ino->d));
	blk_put(b);
	return 0;
}

int inode_write(struct inode *ino)
{
	struct blk_buffer *b;
	int err;
	
	if (ino->sb->ro)
	{
		printf("iwrite %i fs %i,%i\n", ino->ino, major(ino->sb->dev), minor(ino->sb->dev));
		panic("iwrite ro fs");
	}
	
	err = blk_get(ino->sb->dev, ino->ino, &b);
	if (err)
		return err;
	
	memcpy(b->data, &ino->d, sizeof(ino->d));
	b->valid = 1;
	b->dirty = 1;
	blk_put(b);
	ino->dirty = 0;
	return 0;
}

int inode_get(struct super *sb, ino_t ino, struct inode **buf)
{
	struct inode *fr = NULL;
	int err;
	int i;
	
	for (i = 0; i < MAXINODES; i++)
	{
		if (inode[i].refcnt && inode[i].sb == sb && inode[i].ino == ino)
		{
			if (inode[i].mnt_root)
			{
				inode[i].mnt_root->refcnt++;
				*buf = inode[i].mnt_root;
				return 0;
			}
			
			inode[i].refcnt++;
			*buf = &inode[i];
			return 0;
		}
		
		if (!inode[i].refcnt)
			fr = &inode[i];
	}
	
	if (!fr)
	{
		printf("no inodes\n");
		return ENOMEM;
	}
	
	fr->sb		 = sb;
	fr->ino		 = ino;
	fr->dirty	 = 0;
	fr->mnt_dir	 = NULL;
	fr->mnt_root	 = NULL;
	fr->pipe_buf	 = NULL;
	fr->pipe_wrp	 = 0;
	fr->pipe_rdp	 = 0;
	fr->pipe_datlen	 = 0;
	fr->pipe_readers = 0;
	fr->pipe_writers = 0;
	
	err = inode_read(fr);
	if (err)
		return err;
	
	fr->refcnt = 1;
	*buf = fr;
	return 0;
}

int inode_put(struct inode *ino)
{
	int err;
	
	if (!ino)
		return 0;
	
	ino->refcnt--;
	if (!ino->refcnt)
	{
		if (ino->pipe_buf)
			pfree(ino->pipe_buf);
		if (!ino->d.nlink && !ino->sb->ro)
			free_inode(ino);
		
		if (ino->dirty && (err = inode_write(ino)))
		{
			printf("iput %i fs %i,%i\n", ino->ino, major(ino->sb->dev), minor(ino->sb->dev));
			ino->dirty = 0;
			return err;
		}
	}
	
	return 0;
}

void inode_sync(int level)
{
	int i;
	
	for (i = 0; i < MAXINODES; i++)
		if (inode[i].dirty >= level)
			inode_write(&inode[i]);
}

int inode_chkperm4(struct inode *ino, int mode, uid_t uid, gid_t gid)
{
	mode_t perm;
	
	if (ino->sb->ro && (mode & W_OK) && !S_ISCHR(ino->d.mode) && !S_ISBLK(ino->d.mode))
		return EROFS;
	
	if (!uid)
		return 0;
	
	if (uid == ino->d.uid)
		perm = ino->d.mode & S_IRWXU;
	else if (gid == ino->d.gid)
		perm = ino->d.mode & S_IRWXG;
	else
		perm = ino->d.mode & S_IRWXO;
	
	if ((mode & R_OK) && !(perm & (S_IRUSR | S_IRGRP | S_IROTH)))
		return EACCES;
	if ((mode & W_OK) && !(perm & (S_IWUSR | S_IWGRP | S_IWOTH)))
		return EACCES;
	if ((mode & X_OK) && !(perm & (S_IXUSR | S_IXGRP | S_IXOTH)))
		return EACCES;
	return 0;
}

int inode_chkperm(struct inode *ino, int mode)
{
	return inode_chkperm4(ino, mode, curr->euid, curr->egid);
}

int inode_chkperm_rid(struct inode *ino, int mode)
{
	return inode_chkperm4(ino, mode, curr->ruid, curr->rgid);
}
