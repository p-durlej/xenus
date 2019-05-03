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
#include <xenus/console.h>
#include <xenus/config.h>
#include <xenus/page.h>
#include <xenus/umem.h>
#include <xenus/io.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>

extern int cnmode;

static char dmesg[4096];
static int dmesgi;

u16_t *con_fbuf = (u16_t *)PCCON_FBA;
int con_iobase	= PCCON_PORT;
int con_attr	= PCCON_ATTR;
int con_kattr	= PCCON_KATTR;
int con_x;
int con_y;

void con_mono()
{
	con_fbuf   = (u16_t *)MONO_FBA;
	con_iobase = MONO_PORT;
	con_attr   = MONO_ATTR;
	con_kattr  = MONO_KATTR;
}

void con_init()
{
	if (cnmode == 7)
		con_mono();
	con_clear();
}

void con_clear()
{
	int i;
	
	con_x = 0;
	con_y = 0;
	con_setcursor();
	
	for (i = 0; i < PCCON_NCOL * PCCON_NLIN; i++)
		con_fbuf[i] = con_attr | 0x20;
}

void con_setcursor()
{
	int p = con_x + con_y * PCCON_NCOL;
	
	outb(con_iobase + 4, 14);
	outb(con_iobase + 5, p >> 8);
	outb(con_iobase + 4, 15);
	outb(con_iobase + 5, p);
}

void con_chrxy(int x, int y, int ch, int attr)
{
	unsigned i = x + y * PCCON_NCOL;
	
	if (i >= PCCON_NLIN * PCCON_NCOL)
		return;
	
	con_fbuf[i] = (ch & 0xff) | attr;
}

void con_scroll(int attr)
{
	int i;
	
	for (i = 0; i < PCCON_NCOL * (PCCON_NLIN - 1); i++)
		con_fbuf[i] = con_fbuf[i + PCCON_NCOL];
	
	for (i = 0; i < PCCON_NCOL; i++)
		con_fbuf[i + PCCON_NCOL * (PCCON_NLIN - 1)] = attr | 0x20;
}

void con_cscroll(int attr)
{
	while (con_y >= PCCON_NLIN)
	{
		con_scroll(attr);
		con_y--;
	}
}

void con_putca(char ch, int attr)
{
	switch (ch)
	{
	case 0:
		break;
	case '\f':
		con_clear();
		break;
	case '\r':
		con_x = 0;
		break;
	case '\n':
		con_x = 0;
		con_y++;
		con_cscroll(attr);
		break;
	case '\a':
		break;
	case '\b':
		if (con_x)
			con_x--;
		break;
	case '\t':
		con_putca(' ', attr);
		while (con_x & 7)
			con_putca(' ', attr);
		break;
	default:
		if (con_x >= PCCON_NCOL)
		{
			con_x = 0;
			con_y++;
		}
		con_cscroll(attr);
		con_chrxy(con_x, con_y, ch, attr);
		con_x++;
	}
	
	con_setcursor();
}

void con_putsa(char *s, int attr)
{
	while (*s)
	{
		con_putca(*s, attr);
		s++;
	}
}

void con_putk(char ch)
{
	dmesg[dmesgi++] = ch;
	dmesgi %= sizeof dmesg;
	
	return con_putca(ch, con_kattr);
}

void con_putc(char ch)
{
	return con_putca(ch, con_attr);
}

void con_puts(char *s)
{
	while (*s)
	{
		con_putk(*s);
		s++;
	}
}

int sys__dmesg(char *buf, int len)
{
	char *p;
	int err;
	int i;
	
	if (len < sizeof dmesg)
	{
		uerr(EINVAL);
		return -1;
	}
	
	err = uwchk(buf, sizeof dmesg);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	p = buf + USER_BASE;
	i = (dmesgi + 1) % sizeof dmesg;
	do
	{
		if (dmesg[i])
			*p++ = dmesg[i];
		i++;
		i %= sizeof dmesg;
	} while (i != dmesgi);
	
	return p - buf - USER_BASE;
}
