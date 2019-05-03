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
#include <sys/times.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

static void ptime(clock_t t)
{
	unsigned i, f;
	
	f = t % HZ;
	i = t / HZ;
	
	printf("%6i.%02i", i, f * 100 / HZ);
}

int main(int argc, char **argv)
{
	struct tms tms;
	clock_t t0, t;
	pid_t pid;
	int st;
	
	if (argc < 2)
	{
		fputs("time command [arg...]\n", stderr);
		return 1;
	}
	
	t0 = times(NULL);
	pid = fork();
	if (!pid)
	{
		execvp(argv[1], argv + 1);
		perror(argv[1]);
		return 127;
	}
	if (pid < 0)
	{
		perror(NULL);
		return 127;
	}
	
	while (wait(&st) != pid);
	t = times(&tms);
	
	fputs("real ", stdout);
	ptime(t - t0);
	fputs("\nuser ", stdout);
	ptime(tms.tms_cutime);
	fputs("\nsys  ", stdout);
	ptime(tms.tms_cstime);
	putchar('\n');
	
	return 0;
}
