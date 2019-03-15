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
#include <xenus/config.h>
#include <xenus/printf.h>
#include <xenus/clock.h>
#include <xenus/intr.h>
#include <xenus/umem.h>
#include <xenus/fs.h>
#include <xenus/io.h>
#include <sys/dioc.h>
#include <string.h>
#include <errno.h>

#define FD_MAX_RETRIES		3
#define FD_IO_TIMEOUT		(HZ / 2)
#define FD_SEEK_TIMEOUT		(HZ * 3)
#define FD_FORMAT_TIMEOUT	(HZ * 3)

#define FD_SPINUP_DELAY		(1 * HZ)
#define FD_MOTOR_TIMEOUT	(3 * HZ)

#define PHYS_UNITS	2
#define UNITS		2

#define IRQ		6

#define FDC_DOR		0x03f2
#define FDC_MSR		0x03f4
#define FDC_DSR		0x03f4
#define FDC_DATA	0x03f5
#define FDC_DIR		0x03f7
#define FDC_CONFIG	0x03f7

#define DATA_RATE_250	2
#define DATA_RATE_300	1
#define DATA_RATE_500	0
#define DATA_RATE_1000	3

#define NCYL	80
#define NHEAD	2
#define NSECT	18

static volatile int fd_motor_timeout;
static volatile int fd_irq;

static unsigned fdc_dor;
static int need_reset = 1;

static int curr_cyl[PHYS_UNITS];

extern char dma_buf[];

void fd_clock(void)
{
	if (fd_motor_timeout && !--fd_motor_timeout)
	{
		fdc_dor = 0x0c;
		outb(FDC_DOR, fdc_dor);
	}
}

static void fd_irqv()
{
	fd_irq = 1;
}

static unsigned fd_ticks(void)
{
	return time.time * HZ + time.ticks;
}

static int fd_wait(unsigned timeout)
{
	time_t tmout = fd_ticks() + timeout;
	int s;
	
	asm volatile("cli;");
	while (!fd_irq && fd_ticks() < tmout)
		asm volatile("sti; hlt; cli;");
	asm volatile("sti;");
	
	if (!fd_irq)
	{
		need_reset = 1;
		return EIO;
	}
	return 0;
}

static int fd_out(unsigned byte)
{
	time_t tmout = fd_ticks() + FD_IO_TIMEOUT;
	
	while ((inb(FDC_MSR) & 0xc0) != 0x80 && fd_ticks() < tmout);
	if ((inb(FDC_MSR) & 0xc0) != 0x80)
	{
		fd_motor_timeout = FD_MOTOR_TIMEOUT;
		need_reset = 1;
		return EIO;
	}
	
	outb(FDC_DATA, byte);
	return 0;
}

static int fd_in(unsigned *byte)
{
	time_t tmout = fd_ticks() + FD_IO_TIMEOUT;
	
	while ((inb(FDC_MSR) & 0xc0) != 0xc0 && fd_ticks() < tmout);
	if ((inb(FDC_MSR) & 0xc0) != 0xc0)
	{
		need_reset = 1;
		return EIO;
	}
	
	*byte = inb(FDC_DATA);
	return 0;
}

static int fd_reset(void)
{
	unsigned st0, st1;
	int err;
	int i;
	
	for (i = 0; i < PHYS_UNITS; i++)
		curr_cyl[i] = -1;
	
	outb(FDC_DOR, 0);
	delay(1);
	outb(FDC_DOR, fdc_dor | 0x0c);
	
	err = fd_wait(FD_IO_TIMEOUT);
	if (err)
		return err;
	
	for (i = 0; i < 4; i++)
	{
		if (fd_out(0x08))
			continue;
		if (fd_in(&st0))
			continue;
		if ((st0 & 0xc0) != 0x80)
			fd_in(&st1);
	}
	
	outb(FDC_CONFIG, DATA_RATE_500);
	
	fd_out(0x03); /* fix drive data command */
	fd_out(0x0f);
	fd_out(0xfe);
	
	need_reset = 0;
	return 0;
}

static void fd_select(int u)
{
	unsigned motor = 0x10 << u;
	
	fd_motor_timeout = 0;
	
	if (need_reset)
		fd_reset();
	
	if (fdc_dor & motor)
	{
		fdc_dor = motor | u | 0x0c;
		outb(FDC_DOR, fdc_dor);
	}
	else
	{
		fdc_dor = motor | u | 0x0c;
		outb(FDC_DOR, fdc_dor);
		delay(FD_SPINUP_DELAY);
	}
}

static int fd_seek0(int u)
{
	unsigned st0;
	unsigned cyl;
	int err;
	
	fd_irq = 0;
	if ((err = fd_out(0x07)))		return err;
	if ((err = fd_out(u)))			return err;
	if ((err = fd_wait(FD_SEEK_TIMEOUT)))
		return err;
	
	fd_out(0x08);
	if ((err = fd_in(&st0)))		return err;
	if ((err = fd_in(&cyl)))		return err;
	
	if (st0 & 0xc0)
	{
		curr_cyl[u] = -1;
		return EIO;
	}
	curr_cyl[u] = 0;
	return 0;
}

static int fd_seek(unsigned u, unsigned c, unsigned h)
{
	unsigned st0;
	unsigned cyl;
	int err;
	
	fd_irq = 0;
	if ((err = fd_out(0x0f)))		return err;
	if ((err = fd_out(u)))			return err;
	if ((err = fd_out(c)))			return err;
	if ((err = fd_wait(FD_SEEK_TIMEOUT)))	return err;
	
	fd_out(0x08);
	if ((err = fd_in(&st0)))		return err;
	if ((err = fd_in(&cyl)))		return err;
	if (st0 & 0xc0)
	{
		curr_cyl[u] = -1;
		return EIO;
	}
	if (cyl != c)
	{
		curr_cyl[u] = -1;
		return EIO;
	}
	curr_cyl[u] = c;
	return 0;
}

static void fd_setup_dma(int wr)
{
	outb(0x0a, 0x06); /* disable dma channel 2 */
	
	if (wr)
		outb(0x0b, 0x4a); /* dma mode register setup */
	else
		outb(0x0b, 0x46); /* dma mode register setup */
	
	outb(0x0c, 0xff); /* clear byte pointer flip-flop */
	outb(0x04, (unsigned)dma_buf);
	outb(0x04, (unsigned)dma_buf >> 8);
	outb(0x05, 0xff); /* set channel 2 starting word count to 511 */
	outb(0x05, 0x01); /* 8237 wants one less than real word count */
	
	outb(0x81, (unsigned)dma_buf >> 16); /* latch 64K memory page number */
	
	outb(0x0a, 0x02); /* enable dma channel 2 */
}

static int fd_bread(dev_t dev, blk_t blk, void *buf)
{
	int retries = 0;
	unsigned u = minor(dev);
	unsigned st0;
	unsigned st1;
	unsigned st2;
	unsigned c;
	unsigned h;
	unsigned s;
	unsigned t;
	unsigned junk;
	int err;
	
	s = blk % NSECT;
	t = blk / NSECT;
	h = t   % NHEAD;
	c = t   / NHEAD;
	
	if (c >= NCYL)
	{
		printf("fd%i rd bad cyl %i blk %i\n", u, c, blk);
		return EIO;
	}
	
retry:
	if (++retries > FD_MAX_RETRIES)
		goto unlock;
	fd_select(u);
	
	if (curr_cyl[u] < 0)
	{
		err = fd_seek0(u);
		if (err)
			goto retry;
	}
	
	if (curr_cyl[u] != c)
	{
		err = fd_seek(u, c, h);
		if (err)
		{
			curr_cyl[u] = -1;
			goto retry;
		}
	}
	
	fd_setup_dma(0);
	
	fd_irq = 0;
	if ((err = fd_out(0xe6)))		goto retry; /* read sector command */
	if ((err = fd_out(u | (h << 2))))	goto retry;
	if ((err = fd_out(c)))			goto retry;
	if ((err = fd_out(h)))			goto retry;
	if ((err = fd_out(s + 1)))		goto retry;
	if ((err = fd_out(2)))			goto retry;
	if ((err = fd_out(s + 1)))		goto retry;
	if ((err = fd_out(27)))			goto retry;
	if ((err = fd_out(0xff)))		goto retry;
	
	if ((err = fd_wait(FD_IO_TIMEOUT)))	goto retry;
	
	if ((err = fd_in(&st0)))		goto retry;
	if ((err = fd_in(&st1)))		goto retry;
	if ((err = fd_in(&st2)))		goto retry;
	if ((err = fd_in(&junk)))		goto retry;
	if ((err = fd_in(&junk)))		goto retry;
	if ((err = fd_in(&junk)))		goto retry;
	if ((err = fd_in(&junk)))		goto retry;
	
	if (st0 & 0xc0)
	{
		curr_cyl[u] = -1;
		err = EIO;
		goto retry;
	}
	
	memcpy(buf, dma_buf, 512);
unlock:
	fd_motor_timeout = FD_MOTOR_TIMEOUT;
	return err;
}

static int fd_bwrite(dev_t dev, blk_t blk, void *buf)
{
	int retries = 0;
	unsigned u = minor(dev);
	unsigned st0;
	unsigned st1;
	unsigned st2;
	unsigned c;
	unsigned h;
	unsigned s;
	unsigned t;
	unsigned junk;
	int err;
	
	memcpy(dma_buf, buf, 512);
	
	s = blk % NSECT;
	t = blk / NSECT;
	h = t	% NHEAD;
	c = t	/ NHEAD;
	
	if (c >= NCYL)
	{
		printf("fd%i wr bad cyl %i blk %i\n", u, c, blk);
		return EIO;
	}
	
retry:
	if (++retries > FD_MAX_RETRIES)
		goto unlock;
	fd_select(u);
	
	if (curr_cyl[u] < 0)
	{
		err = fd_seek0(u);
		if (err)
			goto retry;
	}
	
	if (curr_cyl[u] != c)
	{
		err = fd_seek(u, c, h);
		if (err)
		{
			curr_cyl[u] = -1;
			goto retry;
		}
	}
	
	fd_setup_dma(1);
	
	fd_irq = 0;
	if ((err = fd_out(0x45)))		goto retry; /* write sector command */
	if ((err = fd_out(u | (h << 2))))	goto retry;
	if ((err = fd_out(c)))			goto retry;
	if ((err = fd_out(h)))			goto retry;
	if ((err = fd_out(s + 1)))		goto retry;
	if ((err = fd_out(2)))			goto retry;
	if ((err = fd_out(s + 1)))		goto retry;
	if ((err = fd_out(27)))			goto retry;
	if ((err = fd_out(0xff)))		goto retry;
	
	if ((err = fd_wait(FD_IO_TIMEOUT)))	goto retry;
	
	if ((err = fd_in(&st0)))		goto retry;
	if ((err = fd_in(&st1)))		goto retry;
	if ((err = fd_in(&st2)))		goto retry;
	if ((err = fd_in(&junk)))		goto retry;
	if ((err = fd_in(&junk)))		goto retry;
	if ((err = fd_in(&junk)))		goto retry;
	if ((err = fd_in(&junk)))		goto retry;
	
	if (st0 & 0xc0)
	{
		curr_cyl[u] = -1;
		err = EIO;
		goto retry;
	}
	
	fd_motor_timeout = FD_MOTOR_TIMEOUT;
unlock:
	return err;
}

static int fd_fmttrk(int u, int c, int h)
{
	unsigned st0, st1, st2;
	unsigned junk;
	int err;
	char *p;
	int i;
	
	if (c < 0 || c >= 80 || h < 0 || h >= 2)
		return EINVAL;
	
	fd_select(u);
	
	if (curr_cyl[u] < 0)
	{
		err = fd_seek0(u);
		if (err)
			goto unlock;
	}
	
	if (curr_cyl[u] != c)
	{
		err = fd_seek(u, c, h);
		if (err)
			goto unlock;
	}
	
	for (p = dma_buf, i = 1; i <= 18; i++)
	{
		*p++ = c;
		*p++ = h;
		*p++ = i;
		*p++ = 2; /* sector size: 512 bytes */
	}
	
	fd_setup_dma(1);
	
	fd_irq = 0;
	if ((err = fd_out(0x4d)))		goto unlock; /* format sector command */
	if ((err = fd_out(u | (h << 2))))	goto unlock;
	if ((err = fd_out(2)))			goto unlock; /* sector size: 512 bytes */
	if ((err = fd_out(18)))			goto unlock; /* number of sectors */
	if ((err = fd_out(0x54)))		goto unlock; /* GAP3 */
	if ((err = fd_out(0x58)))		goto unlock; /* fill byte */
	
	if ((err = fd_wait(FD_FORMAT_TIMEOUT)))	goto unlock;
	
	if ((err = fd_in(&st0)))		goto unlock;
	if ((err = fd_in(&st1)))		goto unlock;
	if ((err = fd_in(&st2)))		goto unlock;
	if ((err = fd_in(&junk)))		goto unlock;
	if ((err = fd_in(&junk)))		goto unlock;
	if ((err = fd_in(&junk)))		goto unlock;
	if ((err = fd_in(&junk)))		goto unlock;
	
	if (st0 & 0xc0)
	{
		printf("fd%i fmt bad cyl %i head %i\n", u, c, h);
		curr_cyl[u] = -1;
		err = EIO;
		goto unlock;
	}
unlock:
	fd_motor_timeout = FD_MOTOR_TIMEOUT;
	return err;
}

static int fd_ioctl(dev_t dev, int cmd, void *ptr)
{
	struct fmttrk fmt;
	struct dinfo di;
	int err;
	
	switch (cmd)
	{
	case DIOCGINFO:
		memset(&di, 0, sizeof di);
		di.c = 80;
		di.h = 2;
		di.s = 18;
		return tucpy(ptr, &di, sizeof di);
	case DIOCFMTTRK:
		err = fucpy(&fmt, ptr, sizeof fmt);
		if (err)
			return err;
		return fd_fmttrk(dev & 1, fmt.c, fmt.h);
	default:
		;
	}
	return ENOTTY;
}

int rfd_ioctl(struct inode *ino, int cmd, void *ptr)
{
	return fd_ioctl(ino->d.rdev, cmd, ptr);
}

int rfd_read(struct rwreq *req)
{
	char *p = req->buf;
	blk_t blk, cnt;
	int err;
	
	if (req->count & 511)
		return EINVAL;
	if (req->start & 511)
		return EINVAL;
	
	err = uwchk(p, req->count);
	if (err)
		return err;
	p += curr->base;
	
	blk = req->start / 512;
	cnt = req->count / 512;
	
	while (cnt--)
	{
		err = fd_bread(req->ino->d.rdev, blk++, p);
		if (err)
			return err;
		p += 512;
	}
	
	req->start += req->count;
	return 0;
}

int rfd_write(struct rwreq *req)
{
	char *p = req->buf;
	blk_t blk, cnt;
	int err;
	
	if (req->count & 511)
		return EINVAL;
	if (req->start & 511)
		return EINVAL;
	
	err = urchk(p, req->count);
	if (err)
		return err;
	p += curr->base;
	
	blk = req->start / 512;
	cnt = req->count / 512;
	
	while (cnt--)
	{
		err = fd_bwrite(req->ino->d.rdev, blk++, p);
		if (err)
			return err;
		p += 512;
	}
	
	req->start += req->count;
	return 0;
}

void fd_init(void)
{
	int i;
	
	irq_set(IRQ, fd_irqv);
	irq_ena(IRQ);
	
	for (i = 0; i < PHYS_UNITS; i++)
		curr_cyl[i] = -1;
	
	outb(FDC_DOR, 0x0c);
	
	blk_driver[FD_DEVN].read  = fd_bread;
	blk_driver[FD_DEVN].write = fd_bwrite;
	blk_driver[FD_DEVN].ioctl = fd_ioctl;
}

void fd_stop(void)
{
	fd_motor_timeout = 0;
	fdc_dor = 0x0c;
	outb(FDC_DOR, fdc_dor);
}

void rfd_init(void)
{
	chr_driver[RFD_DEVN].read  = rfd_read;
	chr_driver[RFD_DEVN].write = rfd_write;
	chr_driver[RFD_DEVN].ioctl = rfd_ioctl;
}
