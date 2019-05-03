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

#include <sys/ioctl.h>
#include <sys/dioc.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

static unsigned defpart[] = { 5000, 2880, 2120, 10000, 10000, 50000, 0 };
static char *devname = "/dev/rhd0";
static int devfd;
static char buf[512];
static unsigned *part = (unsigned *)(buf + 0x1d0);
static int iflag;

static void opendev(void)
{
	devfd = open(devname, O_RDWR);
	if (devfd < 0)
	{
		perror(devname);
		exit(1);
	}
	
	errno = EINVAL;
	if (read(devfd, buf, sizeof buf) != sizeof buf)
	{
		perror(devname);
		exit(1);
	}
}

int main(int argc, char **argv)
{
	int w = 0;
	int i, n;
	
	if (argc > 2 && !strcmp(argv[1], "-D"))
	{
		devname = argv[2];
		argv += 2;
		argc -= 2;
	}
	
	if (argc > 1 && !strcmp(argv[1], "-I"))
	{
		argv += 2;
		argc -= 2;
		iflag = 1;
	}
	
	if (*argv[1] == '-')
	{
		fprintf(stderr, "setpart: Bad option '%c'\n", argv[1][1]);
		return 1;
	}
	
	opendev();
	
	if (iflag)
	{
		memcpy(part, defpart, sizeof defpart);
		w = 1;
	}
	
	if (argc > 8)
	{
		fputs("Too many partitions\n", stderr);
		return 1;
	}
	
	if (argc > 1)
	{
		for (i = 1; argv[i]; i++)
			part[i - 1] = atoi(argv[i]);
		for (; i < 8; i++)
			part[i - 1] = 0;
		w = 1;
	}
	
	printf("%s:", devname);
	for (n = 6; n >= 0; n--)
		if (part[n])
			break;
	for (i = 0; i <= n; i++)
		printf(" %u", part[i]);
	if (n < 0)
		printf(" no partitions");
	putchar('\n');
	
	if (w)
	{
		lseek(devfd, 0, SEEK_SET);
		if (write(devfd, buf, sizeof buf) != sizeof buf)
		{
			perror(devname);
			return 1;
		}
		ioctl(devfd, DIOCRTBL, 0);
	}
	return 0;
}
