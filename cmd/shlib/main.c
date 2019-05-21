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

#include <xenus/exec.h>
#include <environ.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

void __libc_start(char *argenv);

extern void entry;

static int (*umain)(int argc, char **argv, char **envp, void *shl);

static void *globals[6];

static void load(char *pathname, int fd)
{
	struct stat st;
	struct shdr h;
	int sz;
	
	errno = ENOEXEC;
	fstat(fd, &st);
	
	if (read(fd, &h, sizeof h) != sizeof h)
		goto fail;
	
	if (h.base)
		goto fail;
	
	if (brk((void *)h.end))
		goto fail;
	
	lseek(fd, 0, SEEK_SET);
	if (read(fd, 0, st.st_size) != st.st_size)
		goto fail;
	
	close(fd);
	
	sz = sizeof globals;
	if (sz > h.globsz)
		sz = h.globsz;
	
	memcpy((void *)h.globals, globals, sz);
	umain = (void *)h.start;
	return;
fail:
	perror(pathname);
	abort();
}

void _stdfile(FILE **buf)
{
	*buf++ = stdin;
	*buf++ = stdout;
	*buf++ = stderr;
}

int rawmain(char *args, char *env, char *prog, int pfd)
{
	static char msg[] = "Not permitted\n";
	
	if (!prog || pfd < 0)
	{
		write(2, msg, sizeof msg - 1);
		return 1;
	}
	
	globals[0] = &entry;
	globals[1] = &errno;
	globals[2] = &environ;
	globals[3] = stdin;
	globals[4] = stdout;
	globals[5] = stderr;
	
	load(prog, pfd);
	__libc_start(args);
	return 0;
}

int main(int argc, char **argv)
{
	return umain(argc, argv, environ, &entry);
}
