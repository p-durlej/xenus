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

#include <xenus/config.h>
#include <xenus/panic.h>
#include <xenus/clock.h>
#include <xenus/umem.h>
#include <xenus/fs.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

struct blk_buffer blk_buf[BCWIDTH][BCDEPTH];
struct blk_driver blk_driver[MAXBDEVS];

static int blk_nind;

void hd_init();
void fd_init();

#if CDEBUGBLK
void cdebugblk()
{
	struct blk_buffer *b;
	int nd = 0, nv = 0;
	int i, n;
	
	for (i = 0; i < BCWIDTH; i++)
		for (n = 0; n < BCDEPTH; n++)
		{
			b = &blk_buf[i][n];
			
			if (b->dirty)
				nd++;
			if (b->valid)
				nv++;
		}
	printf("nd %i nv %i\n", nd, nv);
}
#endif

void blk_init()
{
	hd_init();
	fd_init();
}

int blk_get(dev_t dev, blk_t blk, struct blk_buffer **buf)
{
	struct blk_buffer *b;
	int i = blk % BCWIDTH;
	int n;
	
	for (n = 0; n < BCDEPTH; n++)
	{
		b = &blk_buf[i][n];
		
		if ((b->refcnt || b->valid) && b->dev == dev && b->blk == blk)
		{
			b->refcnt++;
			*buf = b;
			return 0;
		}
	}
	
	for (n = 0; n < BCDEPTH; n++)
	{
		b = &blk_buf[i][blk_nind++];
		blk_nind %= BCDEPTH;
		
		if (!b->refcnt && !b->dirty)
		{
			b->valid  = 0;
			b->dirty  = 0;
			b->dev	  = dev;
			b->blk	  = blk;
			b->refcnt = 1;
			*buf = b;
			return 0;
		}
	}
	
	for (n = 0; n < BCDEPTH; n++)
	{
		b = &blk_buf[i][blk_nind++];
		blk_nind %= BCDEPTH;
		
		if (!b->refcnt)
		{
			if (b->dirty)
				blk_write(b);
			b->valid  = 0;
			b->dirty  = 0;
			b->dev	  = dev;
			b->blk	  = blk;
			b->refcnt = 1;
			*buf = b;
			return 0;
		}
	}
	
	printf("no bufs blk %i i %i\n", blk, i);
	return ENOMEM;
}

int blk_read(dev_t dev, blk_t blk, struct blk_buffer **buf)
{
	struct blk_buffer *b;
	unsigned drv = major(dev);
	int err;
	
	if (drv >= MAXBDEVS)
		return ENXIO;
	if (!blk_driver[drv].read)
		return ENODEV;
	err = blk_get(dev, blk, &b);
	if (err)
		return err;
	if (b->valid)
	{
		*buf = b;
		return 0;
	}
	err = blk_driver[drv].read(dev, blk, b->data);
	if (err)
	{
		printf("read blk %i err %i dv %i,%i\n",
			b->blk, err, major(b->dev), minor(b->dev));
		blk_put(b);
		return err;
	}
	b->valid = 1;
	*buf = b;
	return 0;
}

int blk_write(struct blk_buffer *buf)
{
	int err;
	
	if (!buf->dirty)
		return 0;
	if (!buf->valid)
		panic("blk not valid");
	buf->dirty = 0;
	if (!blk_driver[major(buf->dev)].write)
		return ENODEV;
	err = blk_driver[major(buf->dev)].write(buf->dev, buf->blk, buf->data);
	if (err)
		printf("write blk %i err %i dv %i,%i\n",
			buf->blk, err, major(buf->dev), minor(buf->dev));
	return err;
}

int blk_put(struct blk_buffer *buf)
{
	int err;
	
	if (!buf->refcnt)
		panic("unused buf");
	
	buf->refcnt--;
	return 0;
}

void blk_sync(int invl, int timeout)
{
	int i, n;
	int t;
	
	if (timeout)
		t = clock_ticks();
	
	for (i = 0; i < BCWIDTH; i++)
		for (n = 0; n < BCDEPTH; n++)
		{
			if (timeout && clock_ticks() - t > timeout)
				return;
			
			if (blk_buf[i][n].dirty)
				blk_write(&blk_buf[i][n]);
			if (invl && !blk_buf[i][n].refcnt)
				blk_buf[i][n].valid = 0;
		}
}

int blk_upread(dev_t dev, blk_t blk, off_t start, int len, char *buf)
{
	struct blk_buffer *b;
	int err;
	
	err = blk_read(dev, blk, &b);
	if (err)
		return err;
	err = tucpy(buf, b->data + start, len);
	blk_put(b);
	return err;
}

int blk_upwrite(dev_t dev, blk_t blk, off_t start, int len, char *buf)
{
	struct blk_buffer *b;
	int err;
	
	if (len == BLK_SIZE)
		err = blk_get(dev, blk, &b);
	else
		err = blk_read(dev, blk, &b);
	if (err)
		return err;
	err = fucpy(b->data + start, buf, len);
	b->valid = 1;
	b->dirty = 1;
	blk_put(b);
	return err;
}
