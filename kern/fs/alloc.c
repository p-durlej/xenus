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
#include <xenus/clock.h>
#include <xenus/umem.h>
#include <xenus/fs.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

int erase_blk(dev_t dev, blk_t blk)
{
	struct blk_buffer *b;
	int err;
	
	err = blk_get(dev, blk, &b);
	if (err)
		return err;
	memset(b->data, 0, sizeof(b->data));
	b->valid = 1;
	b->dirty = 1;
	return blk_put(b);
}

int alloc_blk(struct super *sb, blk_t *blk)
{
	char *fd = freedirt[sb - super];
	char *fm = freemap[sb - super];
	int err;
	blk_t i;
	
	if (sb->ro)
		return EROFS;
	
	if (sb->freeblk < sb->dblock)
		sb->freeblk = sb->dblock;
	
	for (i = sb->freeblk; i < sb->nblocks; i++)
	{
		if (fm[i / 8] & (1 << (i & 7)))
			continue;
		
		err = erase_blk(sb->dev, i);
		if (err)
			return err;
		
		sb->freeblk = i + 1;
		fd[i / 4096] = 1;
		fm[i / 8] |= 1 << (i & 7);
		*blk = i;
		if (sb->nfree != -1)
			sb->nfree--;
		return 0;
	}
	
	printf("fs %i,%i full\n", major(sb->dev), minor(sb->dev));
	return ENOSPC;
}

int free_blk(struct super *sb, blk_t blk)
{
	freedirt[sb - super][blk / 4096] = 1;
	freemap[sb - super][blk / 8] &= ~(1 << (blk & 7));
	
	if (sb->freeblk > blk)
		sb->freeblk = blk;
	
	if (sb->nfree != -1)
		sb->nfree++;
	
	return 0;
}

int alloc_inode(struct super *sb, struct inode **ino)
{
	ino_t i;
	int err;
	
	err = alloc_blk(sb, (blk_t *)&i);
	if (err)
		return err;
	
	err = inode_get(sb, i, ino);
	if (err)
	{
		free_blk(sb, i);
		return err;
	}
	
	(*ino)->d.uid	= curr->euid;
	(*ino)->d.gid	= curr->egid;
	(*ino)->d.rdev	= 0;
	(*ino)->d.size	= 0;
	(*ino)->d.mode	= 0;
	(*ino)->d.blocks= 0;
	(*ino)->d.atime	= time.time;
	(*ino)->d.mtime	= time.time;
	(*ino)->d.ctime	= time.time;
	(*ino)->d.nlink	= 0;
	memset((*ino)->d.bmap0, 0, sizeof((*ino)->d.bmap0));
	memset((*ino)->d.bmap1, 0, sizeof((*ino)->d.bmap1));
	memset((*ino)->d.bmap2, 0, sizeof((*ino)->d.bmap2));
	return 0;
}

int free_inode(struct inode *ino)
{
	int err;
	
	err = free_blk(ino->sb, (blk_t)ino->ino);
	if (err)
		return err;
	return trunc(ino, 0);
}

int count_free(struct super *sb)
{
	struct blk_buffer *b = NULL;
	int err;
	blk_t i, n, k;
	blk_t bn;
	int nb;
	int s;
	
	nb = (sb->nblocks + 4095) / 4096;
	sb->nfree = 0;
	
	for (bn = i = 0; i < nb; i++)
	{
		err = blk_read(sb->dev, sb->bitmap + i, &b);
		if (err)
			return err;
		
		for (k = 0; k < BLK_SIZE; k++)
		{
			s = b->data[k];
			for (n = 0; n < 8; n++, s >>= 1, bn++)
			{
				if (bn >= sb->nblocks)
					goto fini;
				if (s & 1)
					continue;
				
				sb->nfree++;
			}
		}
		
		blk_put(b);
	}
	
fini:
	return 0;
}

int sys_statfs(char *path, struct statfs *buf)
{
	char lpath[PATH_MAX];
	struct inode *ino;
	struct super *sb;
	struct statfs st;
	int err;
	
	err = fustr(lpath, path, PATH_MAX);
	if (err)
		goto fail;
	
	err = dir_traverse(lpath, 0, &ino);
	if (err)
		goto fail;
	
	sb = ino->sb;
	inode_put(ino);
	
	if (sb->nfree == -1)
		err = count_free(sb);
	if (err)
		goto fail;
	st.f_blocks	= sb->nblocks;
	st.f_bfree	= sb->nfree;
	st.f_dev	= sb->dev;
	st.f_ro		= sb->ro;
	err = tucpy(buf, &st, sizeof(st));
	if (err)
		goto fail;
	return 0;
fail:
	uerr(err);
	return -1;
}
