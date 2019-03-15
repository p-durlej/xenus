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

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

static int xit;

static void cat1(char *name)
{
	char buf[512];
	int cnt;
	int fd;
	
	if (name)
	{
		fd = open(name, O_RDONLY);
		if (fd < 0)
		{
			perror(name);
			xit = 1;
			return;
		}
	}
	else
		fd = STDIN_FILENO;
	
	while (cnt = read(fd, buf, sizeof(buf)), cnt)
	{
		if (cnt < 0)
		{
			perror(name);
			xit = 1;
			break;
		}
		
		if (write(STDOUT_FILENO, buf, cnt) != cnt)
		{
			perror("stdout");
			xit = 1;
			break;
		}
	}
	if (fd != STDIN_FILENO)
		close(fd);
}

int main(int argc, char **argv)
{
	int i;
	
	if (argc == 1)
	{
		cat1(NULL);
		return 0;
	}
	
	for (i = 1; i < argc; i++)
		cat1(argv[i]);
	return 0;
}
