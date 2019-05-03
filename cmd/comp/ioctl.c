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
#include <sys/termios.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "mnxcompat.h"

struct mnx_termios
{
	unsigned short iflag, oflag, cflag, lflag;
	unsigned ispeed, ospeed;
	unsigned char cc[20];
};

struct mnx_winsize
{
	unsigned short row, col, xpixel, ypixel;
};

static int badioc(unsigned cmd)
{
#if MNXDEBUG
	mprintf("mnx_ioctl 0x%x\n", cmd);
#endif
	errno = ENOTTY;
	return -1;
}

static int mnx_tcgets(int fd, struct mnx_termios *mt)
{
	struct termios tio;
	
	if (tcgetattr(fd, &tio))
		return -1;
	
	memset(mt, 0, sizeof *mt);
	
	if (tio.c_lflag & ISIG)		mt->lflag |= 0x0040;
	if (tio.c_lflag & ICANON)	mt->lflag |= 0x0010;
	if (tio.c_lflag & ECHO)		mt->lflag |= 0x0001;
	if (tio.c_lflag & ECHOE)	mt->lflag |= 0x0002;
	
	if (tio.c_oflag & OPOST)	mt->oflag |= 0x0001;
	if (tio.c_oflag & ONLCR)	mt->oflag |= 0x0002;
	
	memset(mt->cc, 255, sizeof mt->cc);
	
	mt->cc[0] = tio.c_cc[VEOF];
	mt->cc[2] = tio.c_cc[VERASE];
	mt->cc[3] = tio.c_cc[VINTR];
	mt->cc[4] = tio.c_cc[VKILL];
	mt->cc[6] = tio.c_cc[VQUIT];
	
#if MNXDEBUG
//	mprintf("mnx_tcgets lflag 0x%x oflag 0x%x mlflag 0x%x moflag 0x%x\n",
//		tio.c_lflag, tio.c_oflag, mt->lflag, mt->oflag);
#endif
	return 0;
}

static int mnx_tcsets(int fd, struct mnx_termios *mt)
{
	struct termios tio;
	
	if (tcgetattr(fd, &tio))
		return -1;
	
	tio.c_lflag &= ~(ISIG | ICANON | ECHO | ECHOE);
	tio.c_oflag &= ~(OPOST | ONLCR);
	
	if (mt->lflag & 0x0040) tio.c_lflag |= ISIG;
	if (mt->lflag & 0x0010) tio.c_lflag |= ICANON;
	if (mt->lflag & 0x0001) tio.c_lflag |= ECHO;
	if (mt->lflag & 0x0002) tio.c_lflag |= ECHOE;
	
	if (mt->oflag & 0x0001) tio.c_oflag |= OPOST;
	if (mt->oflag & 0x0002) tio.c_oflag |= ONLCR;
	
	tio.c_cc[VEOF]	 = mt->cc[0];
	tio.c_cc[VERASE] = mt->cc[2];
	tio.c_cc[VINTR]  = mt->cc[3];
	tio.c_cc[VKILL]  = mt->cc[4];
	tio.c_cc[VQUIT]  = mt->cc[6];
	
	return tcsetattr(fd, TCSANOW, &tio);
}

static int mnx_gwinsz(int fd, struct mnx_winsize *mt)
{
	char *ln = getenv("LINES");
	char *tt = getenv("TERM");
	
	mt->xpixel = 640;
	mt->ypixel = 480;
	mt->col = 80;
	mt->row = 24;
	
	if (tt && !strcmp(tt, "xenus"))
		mt->row = 25;
	
	if (ln)
		mt->row = atoi(ln);
	
	return 0;
}

int mnx_ioctl(struct message *msg)
{
	void *data = msg->m2.p1;
	unsigned cmd = msg->m2.i3;
	int fd = msg->m2.i1;
	
	switch (cmd & 0xffff)
	{
	case 0x5408:
		return mnx_tcgets(fd, data);
	case 0x5409: // TCSANOW
	case 0x540a: // TCSADRAIN
	case 0x540b: // TCSAFLUSH
		return mnx_tcsets(fd, data);
	case 0x5410: // TIOCGWINSZ
		return mnx_gwinsz(fd, data);
	default:
		return badioc(cmd);
	}
	return 0;
}
