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
#include <xenus/umem.h>
#include <xenus/intr.h>
#include <xenus/io.h>
#include <xenus/fs.h>
#include <sys/types.h>
#include <sys/dioc.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define offset(dev)	(hd_off[minor(dev) & 31])
#define size(dev)	(hd_size[minor(dev) & 31])
#define unit(dev)	(minor(dev) >> 3)

#define NCTLS	2
#define NDISKS	4

#define CMD_READ	0x20
#define CMD_WRITE	0x30

#define OFFSETS		0, 5000, 7880, 10000, 20000, 50000, 100000, 0

static blk_t hd_size[NDISKS * 8];
static blk_t hd_off[NDISKS * 8];

volatile int hd_intr = 0;

extern int ndisks, dsleep;

struct pcpart
{
	unsigned char boot;
	unsigned char schs[3];
	unsigned char id;
	unsigned char echs[3];
	unsigned start, size;
};

struct ctl
{
	unsigned iobase;
	unsigned irq;
} hd_ctl[NCTLS];

struct disk
{
	struct ctl *	ctl;
	unsigned	unit;
	unsigned	wpcom;
	unsigned	ncyls;
	unsigned	nheads;
	unsigned	nsects;
	blk_t		size;
} hd_disk[NDISKS];

void hd_wait(int i)
{
	while (inb(hd_disk[i].ctl->iobase + 0x0007) & 0x80);
}

void hd_reset(int unit)
{
	int iobase = hd_disk[unit].ctl->iobase;
	
	outb(iobase + 0x0206, 0x04);
	outb(iobase + 0x0206, 0x00);
	hd_wait(unit);
	hd_intr = 0;
	
	if (dsleep)
	{
		outb(iobase + 6, 0xc0 | (hd_disk[unit].unit << 4));
		outb(iobase + 2, 5 * 12); /* 5 * 60 seconds */
		outb(iobase + 7, 0xe2);
		
		hd_wait(unit);
		hd_intr = 0;
	}
}

int hd_geterr(int unit)
{
	int status = inb(hd_disk[unit].ctl->iobase + 0x0007);
	int err;
	
	if ((status & 0xf1) == 0x50)
		return 0;
	
	return inb(hd_disk[unit].ctl->iobase + 0x0001);
}

int hd_io(int unit, int cmd, int c, int h, int s, u16_t *buf)
{
	int iobase = hd_disk[unit].ctl->iobase;
	int i;
	
	if (!hd_disk[unit].ctl)
		return ENODEV;
	
	iobase	= hd_disk[unit].ctl->iobase;
	hd_intr	= 0;
	
	outb(iobase + 0x0001, hd_disk[unit].wpcom);
	outb(iobase + 0x0002, 1);
	outb(iobase + 0x0003, s);
	outb(iobase + 0x0004, c);
	outb(iobase + 0x0005, c >> 8);
	outb(iobase + 0x0006, (hd_disk[unit].unit << 4) | h | 0xa0);
	outb(iobase + 0x0007, cmd);
	
	if (cmd == CMD_WRITE)
	{
		while (!(inb(iobase + 0x0007) & 0x08));
		
		outsw(iobase, buf, 256);
	}
	
	asm volatile("cli");
	while (!hd_intr)
		asm volatile("sti; hlt; cli;");
	asm volatile("sti");
	inb(iobase + 0x0007);
	
	if (hd_geterr(unit))
		return EIO;
	
	if (cmd == CMD_READ)
		insw(iobase, buf, 256);
	
	return 0;
}

int hd_bread(dev_t dev, blk_t blk, void *buf)
{
	int c, h, s, t;
	int unit;
	
	if (blk >= size(dev))
		return EINVAL;
	
	blk += offset(dev);
	unit = unit(dev);
	if (unit >= NDISKS)
		return ENODEV;
	
	if (blk >= hd_disk[unit].size)
		return EIO;
	
	t = blk / hd_disk[unit].nsects;
	s = blk % hd_disk[unit].nsects;
	c = t	/ hd_disk[unit].nheads;
	h = t	% hd_disk[unit].nheads;
	s++;
	
	return hd_io(unit, CMD_READ, c, h, s, (u16_t *)buf);
}

int hd_bwrite(dev_t dev, blk_t blk, void *buf)
{
	int c, h, s, t;
	int unit;
	
	if (blk >= size(dev))
		return EINVAL;
	
	blk += offset(dev);
	unit = unit(dev);
	if (unit >= NDISKS)
		return ENODEV;
	
	if (blk >= hd_disk[unit].size)
		return EIO;
	
	t = blk / hd_disk[unit].nsects;
	s = blk % hd_disk[unit].nsects;
	c = t	/ hd_disk[unit].nheads;
	h = t	% hd_disk[unit].nheads;
	s++;
	
	return hd_io(unit, CMD_WRITE, c, h, s, (u16_t *)buf);
}

void hd_irq(int i)
{
	hd_intr = 1;
}

void hd_conf(int n)
{
	int iobase = hd_ctl[n / 2].iobase;
	int c, h, s, wp;
	u16_t buf[256];
	int t = time.time * HZ + time.ticks;
	int i;
	
	hd_intr = 0;
	
	outb(iobase + 6, (n & 1) << 4);
	outb(iobase + 7, 0xec);
	
	while (time.time * HZ + time.ticks - t <= HZ / 5 && !hd_intr);
	if (!hd_intr || (inb(iobase + 7) & 1))
		return;
	inb(iobase + 7);
	
	for (i = 0; i < BLK_SIZE >> 1; i++)
		buf[i] = inw(iobase);
	
	wp = 65536;
	c = buf[1];
	h = buf[3];
	s = buf[6];
	
	if (!c || !h || !s)
		return;
	
	hd_disk[n].ctl	  = &hd_ctl[n / 2];
	hd_disk[n].unit	  = n % 2;
	hd_disk[n].size	  = c * h * s;
	hd_disk[n].ncyls  = c;
	hd_disk[n].nheads = h;
	hd_disk[n].nsects = s;
	hd_disk[n].wpcom  = wp;
	
	printf("hd%i = %i\n", n * 8, c * h * s);
}

void hd_stop(void)
{
	int i;
	
	for (i = 0; i < NDISKS; i++)
		if (hd_disk[i].ctl)
		{
			int iobase = hd_disk[i].ctl->iobase;
			
			outb(iobase + 6, 0xc0 | (hd_disk[i].unit << 4));
			outb(iobase + 7, 0xe6);
		}
}

void hd_gtable(int unit)
{
	struct pcpart *p, *e;
	char buf[512];
	unsigned *part = (unsigned *)(buf + 0x1d0);
	blk_t sz = hd_disk[unit].size;
	blk_t o = 0;
	int i;
	
	hd_size[7] = hd_disk[unit].size;
	hd_off[7]  = 0;
	
	if (hd_io(unit, CMD_READ, 0, 0, 1, (void *)buf))
		return;
	
	for (p = (void *)(buf + 0x1be), e = p + 4; p < e; p++)
		if (p->id == 0xc0)
		{
			o = p->start;
			sz = p->size;
			printf("hd%i offset %i\n", unit * 8, p->start);
			break;
		}
	hd_size[unit * 8] = sz;
	hd_off [unit * 8] = o;
	
	if (hd_bread(unit * 8, 0, buf))
	{
		printf("hd%i can't read partition table\n", unit * 8);
		return;
	}
	
	for (i = 0; i < 7; i++)
	{
		hd_size[i + unit * 8] = part[i];
		hd_off [i + unit * 8] = o;
		
		o += part[i];
	}
	if (!hd_size[unit * 8])
		hd_size[unit * 8] = 1;
}

int hd_ioctl(dev_t dev, int cmd, void *ptr)
{
	struct disk *dk = &hd_disk[unit(dev)];
	struct dinfo di;
	int err = 0;
	
	switch (cmd)
	{
	case DIOCGINFO:
		memset(&di, 0, sizeof di);
		di.offset = offset(dev);
		di.size	  = size(dev);
		di.c	  = dk->ncyls;
		di.h	  = dk->nheads;
		di.s	  = dk->nsects;
		err = tucpy(ptr, &di, sizeof di);
		break;
	case DIOCRTBL:
		hd_gtable(unit(dev));
		break;
	default:
		return ENOTTY;
	}
	return err;
}

int rhd_ioctl(struct inode *ino, int cmd, void *ptr)
{
	return hd_ioctl(ino->d.rdev, cmd, ptr);
}

int rhd_read(struct rwreq *req)
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
		err = hd_bread(req->ino->d.rdev, blk++, p);
		if (err)
			return err;
		p += 512;
	}
	
	req->start += req->count;
	return 0;
}

int rhd_write(struct rwreq *req)
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
		err = hd_bwrite(req->ino->d.rdev, blk++, p);
		if (err)
			return err;
		p += 512;
	}
	
	req->start += req->count;
	return 0;
}

void hd_init(void)
{
	int i;
	
	hd_ctl[0].iobase = HDC0_IOBASE;
	hd_ctl[0].irq	 = HDC0_IRQ;
	
	hd_ctl[1].iobase = HDC1_IOBASE;
	hd_ctl[1].irq	 = HDC1_IRQ;
	
	irq_set(HDC0_IRQ, hd_irq);
	irq_ena(HDC0_IRQ);
	
	irq_set(HDC1_IRQ, hd_irq);
	irq_ena(HDC1_IRQ);
	
	if (ndisks > NDISKS)
		ndisks = NDISKS;
	
	for (i = 0; i < ndisks; i++)
		hd_conf(i);
	
	for (i = 0; i < ndisks; i++)
		if (hd_disk[i].ctl)
		{
			hd_reset(i);
			hd_gtable(i);
		}
	
	blk_driver[HD_DEVN].read  = hd_bread;
	blk_driver[HD_DEVN].write = hd_bwrite;
	blk_driver[HD_DEVN].ioctl = hd_ioctl;
}

void rhd_init(void)
{
	chr_driver[RHD_DEVN].read  = rhd_read;
	chr_driver[RHD_DEVN].write = rhd_write;
	chr_driver[RHD_DEVN].ioctl = rhd_ioctl;
}
