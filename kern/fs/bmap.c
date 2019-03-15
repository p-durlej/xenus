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

#include <xenus/panic.h>
#include <xenus/fs.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>

#define BBMAP_SIZE	(BLK_SIZE / sizeof(blk_t))

int bmap_i(struct inode *ino, blk_t *blk)
{
	struct blk_buffer *m;
	blk_t mblk;
	blk_t *map;
	int err;
	
	mblk = *blk / BBMAP_SIZE;
	*blk = *blk % BBMAP_SIZE;
	
	if (!ino->d.ibmap[mblk])
	{
		*blk = 0;
		return 0;
	}
	mblk = ino->d.ibmap[mblk];
	err  = blk_read(ino->sb->dev, mblk, &m);
	if (err)
		return err;
	map = (blk_t *)m->data;
	*blk = map[*blk];
	blk_put(m);
	return 0;
}

int bmap_alloc_i(struct inode *ino, blk_t *blk)
{
	struct blk_buffer *m;
	blk_t mblk;
	blk_t *map;
	int err;
	
	mblk = *blk / BBMAP_SIZE;
	*blk = *blk % BBMAP_SIZE;
	
	if (!ino->d.ibmap[mblk])
	{
		blk_t nb;
		
		err = alloc_blk(ino->sb, &nb);
		if (err)
			return err;
		
		err = blk_read(ino->sb->dev, nb, &m);
		if (err)
			return err;
		
		ino->d.ibmap[mblk] = nb;
		ino->d.blocks++;
		ino->dirty = 2;
	}
	else
	{
		mblk = ino->d.ibmap[mblk];
		err  = blk_read(ino->sb->dev, mblk, &m);
	}
	if (err)
		return err;
	map = (blk_t *)m->data;
	if (!map[*blk])
	{
		err = alloc_blk(ino->sb, &map[*blk]);
		if (err)
		{
			blk_put(m);
			return err;
		}
		m->dirty = 1;
		ino->d.blocks++;
		ino->dirty = 2;
	}
	*blk = map[*blk];
	blk_put(m);
	return 0;
}

int bmap(struct inode *ino, blk_t *blk)
{
	blk_t map;
	int err;
	
	if (*blk >= BMAP_SIZE)
	{
		(*blk) -= BMAP_SIZE;
		return bmap_i(ino, blk);
	}
	
	*blk = ino->d.bmap[*blk];
	return 0;
}

int bmap_alloc(struct inode *ino, blk_t *blk)
{
	blk_t map;
	int err;
	
	if (ino->sb->ro)
		panic("bmap_alloc ro fs");
	
	if (*blk >= BMAP_SIZE)
	{
		(*blk) -= BMAP_SIZE;
		return bmap_alloc_i(ino, blk);
	}
	
	if (!ino->d.bmap[*blk])
	{
		err = alloc_blk(ino->sb, &ino->d.bmap[*blk]);
		if (err)
			return err;
		ino->d.blocks++;
		ino->dirty = 2;
	}
	
	*blk = ino->d.bmap[*blk];
	return 0;
}

int bmget(struct inode *ino, blk_t blk, struct blk_buffer **buf)
{
	int err;
	
	err = bmap(ino, &blk);
	if (err)
		return err;
	if (blk)
		return blk_read(ino->sb->dev, blk, buf);
	*buf = NULL;
	return 0;
}

int bmget_alloc(struct inode *ino, blk_t blk, struct blk_buffer **buf)
{
	int err;
	
	err = bmap_alloc(ino, &blk);
	if (err)
		return err;
	if (blk)
		return blk_read(ino->sb->dev, blk, buf);
	*buf = NULL;
	return 0;
}

int trunc_i(struct super *sb, blk_t blk, int umap)
{
	struct blk_buffer *b;
	int lasterr = 0;
	blk_t *map;
	int err;
	int i;
	
	err = blk_read(sb->dev, blk, &b);
	if (err)
		return err;
	
	map = (blk_t *)b->data;
	for (i = 0; i < BLK_SIZE / sizeof(blk_t); i++)
		if (map[i])
		{
			err = free_blk(sb, map[i]);
			if (err)
				lasterr = err;
			
			if (umap)
			{
				map[i] = 0;
				b->dirty = 2;
			}
		}
	
	blk_put(b);
	
	return lasterr;
}

int trunc(struct inode *ino, int umap)
{
	int lasterr = 0;
	int err;
	int i;
	
	for (i = 0; i < BMAP_SIZE; i++)
		if (ino->d.bmap[i])
		{
			err = free_blk(ino->sb, ino->d.bmap[i]);
			ino->d.bmap[i] = 0;
			if (err)
				lasterr = err;
		}
	
	for (i = 0; i < IBMAP_SIZE; i++)
		if (ino->d.ibmap[i])
		{
			err = trunc_i(ino->sb, ino->d.ibmap[i], umap);
			if (err)
				lasterr = err;
			
			err = free_blk(ino->sb, ino->d.ibmap[i]);
			if (err)
				lasterr = err;
			ino->d.ibmap[i] = 0;
		}
	
	ino->dirty	= 2;
	ino->d.size	= 0;
	ino->d.blocks	= 0;
	return lasterr;
}
