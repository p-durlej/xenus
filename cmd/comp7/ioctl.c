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
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

#include "v7compat.h"

struct v7_termios
{
	char ispeed, ospeed;
	char erase, kill;
	int flags;
};

static int badioc(unsigned cmd)
{
#if V7DEBUG
	mprintf("v7_ioctl 0x%x\n", cmd);
#endif
	errno = ENOTTY;
	return -1;
}

static int v7_tiocgetp(int fd, struct v7_termios *tp7)
{
	struct termios tio;
	
	if (tcgetattr(fd, &tio))
		return -1;
	
	memset(tp7, 0, sizeof *tp7);
	tp7->flags = 040;
	
	if (tio.c_lflag & ICANON)	tp7->flags &= ~002;
	if (tio.c_lflag & ECHO)		tp7->flags |=  010;
	if (tio.c_oflag & ONLCR)	tp7->flags |=  020;
	if (tio.c_lflag & ISIG)		tp7->flags &= ~040;
	
	tp7->erase = tio.c_cc[VERASE];
	tp7->kill  = tio.c_cc[VKILL];
	
	tp7->ispeed = tp7->ospeed = 13; // B9600
	
	return 0;
}

static int v7_tiocsetp(int fd, struct v7_termios *tp7)
{
	struct termios tio;
	
	if (tcgetattr(fd, &tio))
		return -1;
	
	tio.c_lflag &= ~(ECHO | ECHOE);
	tio.c_lflag |=	 ICANON | ISIG;
	tio.c_oflag &= ~(OPOST | ONLCR);
	
	if (tp7->flags & 002) tio.c_lflag &= ~ ICANON;
	if (tp7->flags & 010) tio.c_lflag |=   ECHO | ECHOE;
	if (tp7->flags & 020) tio.c_oflag |=   OPOST | ONLCR;
	if (tp7->flags & 040) tio.c_lflag &= ~(ICANON | ISIG);
	
	tio.c_cc[VERASE] = tp7->erase;
	tio.c_cc[VKILL]  = tp7->kill;
	
	return tcsetattr(fd, TCSANOW, &tio);
}

int v7_ioctl(int fd, int cmd, void *data)
{
#if V7DEBUG
//	mprintf("v7_ioctl(%i, 0x%x, %p)\n", fd, cmd, data);
#endif
	switch (cmd & 0xffff)
	{
	case 0x7408:
		return v7_tiocgetp(fd, data);
	case 0x7409:
	case 0x740a:
		return v7_tiocsetp(fd, data);
	case 0x6601:
	case 0x6602:
		return fcntl(fd, F_SETFD, cmd == 0x6601);
	default:
		return badioc(cmd);
	}
	return 0;
}
