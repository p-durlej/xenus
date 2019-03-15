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
#include <xenus/ttyld.h>
#include <xenus/umem.h>
#include <xenus/fs.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

static int ttyld_output(struct tty *tty, int c, int nodelay);

void ttyld_init(struct tty *tty)
{
	memset(&tty->termios, 0, sizeof(tty->termios));
	
	tty->termios.c_lflag	   = ECHO | ECHOE | ICANON | ISIG | CRTBS;
	tty->termios.c_oflag	   = OPOST | ONLCR;
	tty->termios.c_ispeed	   = 9600;
	tty->termios.c_ospeed	   = 9600;
	tty->unread		   = NULL;
	tty->inplen		   = 0;
	tty->termios.c_cc[VERASE]  = VBOTH;
	tty->termios.c_cc[VKILL]   = 'U'  & 31;
	tty->termios.c_cc[VINTR]   = 'C'  & 31;
	tty->termios.c_cc[VQUIT]   = '\\' & 31;
	tty->termios.c_cc[VEOF]    = 'D'  & 31;
}

static void ttyld_echoe(struct tty *tty, char ch)
{
	int lf = tty->termios.c_lflag;
	
	if (!(lf & ECHO))
		return;
	
	if (lf & ECHOE)
	{
		ttyld_output(tty, '\b', 1);
		ttyld_output(tty, ' ', 1);
		ttyld_output(tty, '\b', 1);
		return;
	}
	
	if (lf & CRTBS)
	{
		ttyld_output(tty, '\b', 1);
		return;
	}
	
	ttyld_output(tty, ch, 1);
}

static int ttyld_output(struct tty *tty, int c, int nodelay)
{
	int of = tty->termios.c_oflag;
	
	if ((of & (OPOST | ONLCR)) == (OPOST | ONLCR) && c == '\n')
		tty->write(tty, '\r', nodelay);
	return tty->write(tty, c, nodelay);
}

static int ttyld_cmpc(char c, char cc)
{
	if (cc == VBOTH && (c == '\b' || c == '\177'))
		return 1;
	if (cc == VDISABLE)
		return 0;
	return c == cc;
}

int ttyld_input(struct tty *tty, int nodelay)
{
	int lf = tty->termios.c_lflag;
	int err;
	int i;
	char c;
	
	tty->unread = NULL;
	tty->inplen = 0;
	
	for (;;)
	{
		if (err = tty->read(tty, &c, nodelay), err)
			return err;
		
		if (!(tty->termios.c_lflag & ICANON))
		{
			if (tty->termios.c_lflag & ECHO)
				ttyld_output(tty, c, 1);
			tty->unread = tty->buf;
			tty->inplen = 1;
			*tty->buf   = c;
			return 0;
		}
		
		if (c == '\r')
			c = '\n';
		
		if (ttyld_cmpc(c, tty->termios.c_cc[VERASE]))
		{
			if (!tty->inplen)
				continue;
			
			ttyld_echoe(tty, c);
			tty->inplen--;
			continue;
		}
		
		if (ttyld_cmpc(c, tty->termios.c_cc[VKILL]))
		{
			if (lf & ECHO)
			{
				if (lf & (CRTBS | ECHOE))
					for (i = 0; i < tty->inplen; i++)
						ttyld_echoe(tty, '\b');
				else
					ttyld_output(tty, '\n', 1);
			}
			tty->inplen = 0;
			continue;
		}
		
		if (ttyld_cmpc(c, tty->termios.c_cc[VEOF]))
		{
			tty->unread = tty->buf;
			return 0;
		}
		
		if (tty->inplen >= sizeof tty->buf)
			continue;
		
		if (tty->termios.c_lflag & ECHO)
			ttyld_output(tty, c, 1);
		
		tty->buf[tty->inplen++] = c;
		
		if (c == '\n')
		{
			tty->unread = tty->buf;
			return 0;
		}
	}
}

int ttyld_read(struct tty *tty, struct rwreq *rq)
{
	int err;
	
	if (!tty->carrier)
		return ENXIO;
	
	if (!tty->unread)
	{
		err = ttyld_input(tty, rq->nodelay);
		if (err == EAGAIN && rq->nodelay)
		{
			rq->count = 0;
			return 0;
		}
		if (err)
			return err;
	}
	
	if (rq->count > tty->inplen)
		rq->count = tty->inplen;
	tucpy(rq->buf, tty->unread, rq->count);
	tty->inplen -= rq->count;
	tty->unread += rq->count;
	if (!tty->inplen)
		tty->unread = NULL;
	return 0;
}

int ttyld_write(struct tty *tty, struct rwreq *rq)
{
	size_t count;
	char *buf;
	int err;
	char c;
	
	if (!tty->carrier)
		return ENXIO;
	
	count = rq->count;
	buf   = rq->buf;
	
	while (count--)
	{
		fucpy(&c, buf++, 1);
		
		err = ttyld_output(tty, c, rq->nodelay);
		if (err)
			return err;
	}
	return 0;
}

int ttyld_ioctl(struct tty *tty, int cmd, void *ptr)
{
	struct termios tio;
	int xhup = 0;
	int err;
	
	switch (cmd)
	{
	case TCSETA:
		err = fucpy(&tio, ptr, sizeof(tty->termios));
		if (err)
			return err;
		
		if (tio.c_ispeed != tio.c_ospeed)
			return EINVAL;
		
		if (curr->euid && (tty->termios.c_lflag & XHUP))
			xhup = 1;
		
		if (tty->termios.c_ispeed != tio.c_ispeed && tty->speed && !xhup)
			tio.c_ispeed = tty->speed(tty, tio.c_ispeed);
		else
			tio.c_ispeed = tty->termios.c_ispeed;
		tio.c_ospeed = tio.c_ispeed;
		
		if (curr->euid)
		{
			tio.c_lflag &= ~XHUP;
			tio.c_lflag |= tty->termios.c_lflag & XHUP;
		}
		tty->termios = tio;
		return 0;
	case TCGETA:
		return tucpy(ptr, &tty->termios, sizeof(tty->termios));
	default:
		return ENOTTY;
	}
}

static void ttyld_kill(dev_t rdev, int sig)
{
	struct inode *tty;
	int i;
	
	for (i = 0; i < npact; i++)
	{
		tty = pact[i]->tty;
		
		if (tty && tty->d.rdev == rdev)
			sendsig(pact[i], sig);
	}
}

int ttyld_intr(struct tty *tty, int c)
{
	int sig;
	
	if (c == ('X' & 31))
		sig = SIGHUP;
	else if (ttyld_cmpc(c, tty->termios.c_cc[VQUIT]))
		sig = SIGQUIT;
	else if (ttyld_cmpc(c, tty->termios.c_cc[VINTR]))
		sig = SIGINT;
	else
		return 0;
	
	if (!(tty->termios.c_lflag & ISIG) && sig != SIGHUP)
		return 0;
	if (!(tty->termios.c_lflag & XHUP) && sig == SIGHUP)
		return 0;
	
	ttyld_kill(tty->dev, sig);
	return 1;
}

void ttyld_carrier(struct tty *tty, int cd)
{
	int i;
	
	if (tty->carrier == cd)
		return;
	
	tty->carrier = cd;
	wakeup();
	
	if (!cd)
		ttyld_kill(tty->dev, SIGHUP);
}
