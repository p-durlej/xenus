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
#include <xenus/panic.h>
#include <xenus/ttyld.h>
#include <xenus/intr.h>
#include <xenus/io.h>
#include <xenus/fs.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define BUF_SIZE	82

struct unit
{
	int		iobase;
	int		irq;
	char *		obuf;
	volatile char *	ibuf;
	volatile int	ocnt;
	int		otail;
	volatile int	ohead;
	volatile int	icnt;
	int		ihead;
	volatile int	itail;
	struct tty	ttyld;
	int		local;
	int		open;
} rs_units[2];

#if CDEBUGRS
void cdebugrs(void)
{
	int i;
	
	for (i = 0; i < sizeof rs_units / sizeof *rs_units; i++)
		printf("tty%i ocnt %i otail %i ohead %i icnt %i itail %i ihead %i\n",
			i,
			rs_units[i].ocnt,
			rs_units[i].otail,
			rs_units[i].ohead,
			rs_units[i].icnt,
			rs_units[i].itail,
			rs_units[i].ihead);
}
#endif

static void rs_irq(int nr)
{
	struct unit *u;
	unsigned iir;
	int c;
	
	if (nr == rs_units[0].irq)
		u = &rs_units[0];
	else
		u = &rs_units[1];
	
	intr_disable();
	
	while (iir = inb(u->iobase + 2), !(iir & 1))
		switch (iir & 7)
		{
		case 0: /* modem status */
			c = (inb(u->iobase + 6) >> 7) & 1;
			ttyld_carrier(&u->ttyld, c || u->local);
			break;
		case 2: /* transmitter empty */
			if (u->ocnt)
			{
				outb(u->iobase, u->obuf[u->ohead++]);
				u->ohead %= BUF_SIZE;
				u->ocnt--;
			}
			break;
		case 4: /* received data */
			c = inb(u->iobase);
			
			if (ttyld_intr(&u->ttyld, c))
				break;
			
			if (u->icnt < BUF_SIZE)
			{
				u->ibuf[u->itail++] = c;
				u->itail %= BUF_SIZE;
				u->icnt++;
			}
			else
				inb(u->iobase);
			break;
		case 6: /* line status */
			inb(u->iobase + 5);
			break;
		}
	
	intr_enable();
}

static int rs_sspeed(struct unit *u, int bps)
{
	int div;
	
	if (bps < 150)
		bps = 150;
	div = 115200 / bps;
	
	intr_disable();
	outb(u->iobase + 3, inb(u->iobase + 3) | 128);
	outb(u->iobase    , div);
	outb(u->iobase + 1, div >> 8);
	outb(u->iobase + 3, inb(u->iobase + 3) & 127);
	intr_enable();
	
	return 115200 / div;
}

int rs_ttyname(struct inode *ino, char *buf)
{
	strcpy(buf, "/dev/ttyXX");
	buf[8] = '0' + (ino->d.rdev & 1);
	buf[9] = (ino->d.rdev & 8) ? 'l' : 'r';
	return 0;
}

int rs_ioctl(struct inode *ino, int cmd, void *ptr)
{
	struct unit *u = &rs_units[ino->d.rdev & 1];
	
	return ttyld_ioctl(&u->ttyld, cmd, ptr);
}

int rs_open(struct inode *ino, int nodelay)
{
	struct unit *u = &rs_units[ino->d.rdev & 1];
	
	if (ino->d.rdev != u->ttyld.dev && u->open)
		return EBUSY;
	
	u->local = (ino->d.rdev >> 3) & 1;
	u->ttyld.dev = ino->d.rdev;
	u->open++;
	if (u->local)
	{
		ttyld_carrier(&u->ttyld, 1);
		return 0;
	}
	
	while (!u->ttyld.carrier)
	{
		if (nodelay)
		{
			u->open--;
			return EAGAIN;
		}
		if (curr->sig)
		{
			u->open--;
			return EINTR;
		}
		idle();
	}
	return 0;
}

int rs_close(struct inode *ino)
{
	struct unit *u = &rs_units[ino->d.rdev & 1];
	
	u->open--;
	return 0;
}

int rs_read(struct rwreq *req)
{
	struct unit *u = &rs_units[req->ino->d.rdev & 1];
	
	return ttyld_read(&u->ttyld, req);
}

int rs_write(struct rwreq *req)
{
	struct unit *u = &rs_units[req->ino->d.rdev & 1];
	
	return ttyld_write(&u->ttyld, req);
}

int rs_cread(struct tty *tty, char *c, int nodelay)
{
	struct unit *u = &rs_units[tty->dev & 1];
	
	while (!u->icnt)
	{
		if (nodelay)
			return EAGAIN;
		if (curr->sig)
			return EINTR;
		idle();
	}
	
	*c = u->ibuf[u->ihead++];
	u->ihead %= BUF_SIZE;
	
	intr_disable();
	u->icnt--;
	intr_enable();
	
	return 0;
}

int rs_cwrite(struct tty *tty, char c, int nodelay)
{
	struct unit *u = &rs_units[tty->dev & 1];
	
	if (curr->sig)
		return EINTR;
	
	while (u->ocnt >= BUF_SIZE && nodelay)
		return EAGAIN;
	
	while (u->ocnt >= BUF_SIZE)
	{
		if (curr->sig)
			return EINTR;
		idle();
	}
	
	intr_disable();
	
	u->obuf[u->otail++] = c;
	u->otail %= BUF_SIZE;
	u->ocnt++;
	
	if (inb(u->iobase + 5) & 0x20)
	{
		outb(u->iobase, u->obuf[u->ohead++]);
		u->ohead %= BUF_SIZE;
		u->ocnt--;
	}
	
	intr_enable();
	return 0;
}

int rs_speed(struct tty *tty, int speed)
{
	struct unit *u = &rs_units[tty->dev & 1];
	
	return rs_sspeed(u, speed);
}

void rs_init1(struct unit *u, int iobase, int irq)
{
	u->iobase = iobase;
	u->irq	  = irq;
	
	u->obuf = malloc(BUF_SIZE);
	u->ibuf = malloc(BUF_SIZE);
	
	if (!u->obuf || !u->ibuf)
		panic("rs buf");
	
	outb(u->iobase + 3, 3);
	outb(u->iobase + 1, 11);
	outb(u->iobase + 2, 0);
	outb(u->iobase + 4, 11);
	rs_sspeed(u, 115200);
	rs_irq(u->irq);
	
	irq_set(u->irq, rs_irq);
	irq_ena(u->irq);
	
	u->ttyld.dev	= makedev(RS_DEVN, u - rs_units);
	u->ttyld.read	= rs_cread;
	u->ttyld.write	= rs_cwrite;
	u->ttyld.speed	= rs_speed;
	ttyld_init(&u->ttyld);
	ttyld_carrier(&u->ttyld, (inb(u->iobase + 6) >> 7) & 1);
	
	rs_sspeed(u, u->ttyld.termios.c_ispeed);
}

void rs_init(void)
{
	rs_init1(&rs_units[0], RS0_IOBASE, RS0_IRQ);
	rs_init1(&rs_units[1], RS1_IOBASE, RS1_IRQ);
	
	chr_driver[RS_DEVN].ttyname	= rs_ttyname;
	chr_driver[RS_DEVN].open	= rs_open;
	chr_driver[RS_DEVN].close	= rs_close;
	chr_driver[RS_DEVN].read	= rs_read;
	chr_driver[RS_DEVN].write	= rs_write;
	chr_driver[RS_DEVN].ioctl	= rs_ioctl;
}
