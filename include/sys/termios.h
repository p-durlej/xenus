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

#ifndef _SYS_TERMIOS_H
#define _SYS_TERMIOS_H

#define TCGETA	1
#define TCSETA	2

typedef unsigned char	cc_t;
typedef unsigned int	tcflag_t;

#define ISIG	1
#define ICANON	2
#define ECHO	4
#define ECHOE	8
#define XHUP	16
#define CRTBS	32

#define OPOST	1
#define ONLCR	2

#define VERASE	0
#define VKILL	1
#define VINTR	2
#define VQUIT	3
#define VEOF	4
#define NCCS	5

#define VBOTH	 '\376'
#define VDISABLE '\377'

struct termios
{
	cc_t	 c_cc[NCCS];
	tcflag_t c_cflag;
	tcflag_t c_iflag;
	tcflag_t c_lflag;
	tcflag_t c_oflag;
	unsigned c_ispeed;
	unsigned c_ospeed;
};

#define TCSANOW	1

int tcsetattr(int fd, int flags, struct termios *tp);
int tcgetattr(int fd, struct termios *tp);
int cfsetispeed(struct termios *tp, int speed);
int cfsetospeed(struct termios *tp, int speed);
int cfgetispeed(struct termios *tp);
int cfgetospeed(struct termios *tp);

#endif
