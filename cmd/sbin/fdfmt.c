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
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

static char *	path = "/dev/fd0";
static int	fd;

static void fmttrack(int track, int side)
{
	struct fmttrk fmt;
	int retries = 0;
	
	printf("\rFormatting track %i, side %i ...", track, side);
	fflush(stdout);
	
	memset(&fmt, 0, sizeof fmt);
	fmt.c = track;
	fmt.h = side;
	
retry:
	if (ioctl(fd, DIOCFMTTRK, &fmt))
	{
		if (errno == EIO && ++retries <= 5)
			goto retry;
		putchar('\n');
		perror(path);
		exit(1);
	}
}

int main(int argc, char **argv)
{
	int i;
	
	if (argc > 1)
		path = argv[1];
	
	fd = open(path, O_RDWR);
	if (fd < 0)
	{
		perror(path);
		return 1;
	}
	
	for (i = 0; i < 80; i++)
	{
		fmttrack(i, 0);
		fmttrack(i, 1);
	}
	
	printf("\r                               \r");
	printf("Formatting completed.\n");
	return 0;
}
