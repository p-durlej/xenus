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

#include <xenus/printf.h>
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
	
	if (!ino->d.bmap1[mblk])
	{
		*blk = 0;
		return 0;
	}
	mblk = ino->d.bmap1[mblk];
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
	
	if (!ino->d.bmap1[mblk])
	{
		blk_t nb;
		
		err = alloc_blk(ino->sb, &nb);
		if (err)
			return err;
		
		err = blk_read(ino->sb->dev, nb, &m);
		if (err)
			return err;
		
		ino->d.bmap1[mblk] = nb;
		ino->d.blocks++;
		ino->dirty = 2;
	}
	else
	{
		mblk = ino->d.bmap1[mblk];
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

static int bmindir(struct inode *ino, blk_t mb, blk_t bn, int level, int alloc, blk_t *obp)
{
	struct blk_buffer *mbuf;
	blk_t *map;
	blk_t mi;
	int err;
	
	if (!mb)
		panic("bmindir mb");
	
	err = blk_read(ino->sb->dev, mb, &mbuf);
	if (err)
		return err;
	map = (void *)mbuf->data;
	mi = bn >> (level * 7) & 127;
	
	if (!map[mi] && alloc)
	{
		err = alloc_blk(ino->sb, &map[mi]);
		if (err)
			return err;
		ino->d.blocks++;
		ino->dirty = 2;
		mbuf->dirty = 1;
	}
	
	if (!map[mi])
	{
		*obp = 0;
		return 0;
	}
	
	if (level)
		bmindir(ino, map[mi], bn, level - 1, alloc, &bn);
	
	*obp = map[mi];
	return 0;
}

static int bmap6(struct inode *ino, blk_t bn, blk_t *map, int mapsz, int level, int alloc, blk_t *obp)
{
	blk_t mi;
	int err;
	int i;
	
	mi = bn >> (level * 7);
	
	if (mi >= mapsz)
		return EFBIG;
	
	if (!map[mi] && alloc)
	{
		err = alloc_blk(ino->sb, &map[mi]);
		if (err)
			return err;
		ino->d.blocks++;
		ino->dirty = 2;
	}
	
	if (!map[mi])
	{
		*obp = 0;
		return 0;
	}
	
	if (level)
		return bmindir(ino, map[mi], bn, level - 1, alloc, obp);
	
	*obp = map[mi];
	return 0;
}

int bmap3(struct inode *ino, blk_t *blk, int alloc)
{
	blk_t bn = *blk;
	int err;
	
	if (bn >= BMAP0_SIZE + BMAP1_SIZE * 128)
		return bmap6(ino, bn - BMAP0_SIZE + BMAP1_SIZE * 128, ino->d.bmap2, BMAP2_SIZE, 2, alloc, blk);
	
	if (bn >= BMAP0_SIZE)
		return bmap6(ino, bn - BMAP0_SIZE, ino->d.bmap1, BMAP1_SIZE, 1, alloc, blk);
	
	return bmap6(ino, bn, ino->d.bmap0, BMAP0_SIZE, 0, alloc, blk);
}

int bmap(struct inode *ino, blk_t *blk)
{
	return bmap3(ino, blk, 0);
}

int bmap_alloc(struct inode *ino, blk_t *blk)
{
	return bmap3(ino, blk, 1);
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
	blk_t lbn = blk;
	int err;
	
	err = bmap_alloc(ino, &blk);
	if (err)
		return err;
	if (!blk)
	{
		printf("bmget_alloc ino %u lbn %u blk %u\n", ino->ino, lbn, blk);
		panic("bmget_alloc blk");
	}
	return blk_read(ino->sb->dev, blk, buf);
}

int trunc_i(struct super *sb, blk_t blk, int umap, int level)
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
			if (level)
			{
				err = trunc_i(sb, map[i], umap, level - 1);
				if (err)
					return err;
			}
			
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
	
	for (i = 0; i < BMAP0_SIZE; i++)
		if (ino->d.bmap0[i])
		{
			err = free_blk(ino->sb, ino->d.bmap0[i]);
			ino->d.bmap0[i] = 0;
			if (err)
				lasterr = err;
		}
	
	for (i = 0; i < BMAP1_SIZE; i++)
		if (ino->d.bmap1[i])
		{
			err = trunc_i(ino->sb, ino->d.bmap1[i], umap, 0);
			if (err)
				lasterr = err;
			
			err = free_blk(ino->sb, ino->d.bmap1[i]);
			if (err)
				lasterr = err;
			ino->d.bmap1[i] = 0;
		}
	
	for (i = 0; i < BMAP2_SIZE; i++)
		if (ino->d.bmap2[i])
		{
			err = trunc_i(ino->sb, ino->d.bmap2[i], umap, 1);
			if (err)
				lasterr = err;
			
			err = free_blk(ino->sb, ino->d.bmap2[i]);
			if (err)
				lasterr = err;
			ino->d.bmap2[i] = 0;
		}
	
	ino->dirty	= 2;
	ino->d.size	= 0;
	ino->d.blocks	= 0;
	return lasterr;
}
