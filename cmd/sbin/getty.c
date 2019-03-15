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

#include <sys/termios.h>
#include <sys/ioctl.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

int _ctty(char *path);

int speeds[] =
{
	150,
	300,
	1200,
	2400,
	4800,
	9600,
	19200,
	38400,
	57600,
	115200,
};

struct termios tio =
{
	.c_lflag  = ICANON | ECHO | ECHOE | ISIG | CRTBS,
	.c_oflag  = OPOST | ONLCR,
	.c_ispeed = 9600,
	.c_ospeed = 9600,
	.c_cc     = { VBOTH, 'U' & 31, 'C' & 31, '\\' & 31, 4 },
};

int main(int argc,char **argv)
{
	char ttyp[PATH_MAX];
	int t;
	int d;
	int i;
	
	if (geteuid())
	{
		fputs("Not permitted\n", stderr);
		return 1;
	}
	
	if (argc != 3)
	{
		fputs("getty tty type\n", stderr);
		return 1;
	}
	strcpy(ttyp, "/dev/");
	strcat(ttyp, argv[1]);
	t = *argv[2];
	
	if (t >= '0' && t <= '9')
		tio.c_ispeed = tio.c_ospeed = speeds[t - '0'];
	if (t >= 'a' && t <= 'j')
	{
		tio.c_ispeed = tio.c_ospeed = speeds[t - 'a'];
		tio.c_lflag |= XHUP;
	}
	if (t == 'C')
		tio.c_lflag |= ECHOE | XHUP;
	
	if (_ctty(ttyp))
	{
		perror(argv[1]);
		sleep(10);
		return 1;
	}
	
	for (i = 0; i < OPEN_MAX; i++)
		close(i);
	
	if (open(ttyp, O_RDONLY) != 0) return 1;
	if (open(ttyp, O_WRONLY) != 1) return 1;
	if (open(ttyp, O_WRONLY) != 2) return 1;
	
	ioctl(0, TCSETA, &tio);
	
	execl("/bin/login", "login", (void *)0);
	sleep(10);
	return 1;
}
