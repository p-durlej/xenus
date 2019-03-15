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

#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

int main(int argc, char **argv)
{
	int sig = SIGTERM;
	int err = 0;
	int i = 1;
	
	if (argc == 2 && !strcmp(argv[1], "-l"))
	{
		printf("\n"
			" Signals:\n\n"
			"  SIGHUP ..... 1   SIGSEGV .... 11\n"
			"  SIGINT ..... 2   SIGSYS ..... 12\n"
			"  SIGQUIT .... 3   SIGPIPE .... 13\n"
			"  SIGILL ..... 4   SIGALRM .... 14\n"
			"  SIGTRAP .... 5   SIGTERM .... 15\n"
			"  SIGABRT .... 6   SIGUSR1 .... 16\n"
			"  SIGEMT ..... 7   SIGUSR2 .... 17\n"
			"  SIGFPE ..... 8   SIGCHLD .... 18\n"
			"  SIGKILL .... 9   SIGPWR ..... 19\n"
			"  SIGBUS ..... 10\n\n"
			);
		return 0;
	}
	
	if (argc < 2)
	{
		fputs("kill [-sig] pid1...\n", stderr);
		fputs("kill -l\n", stderr);
		return 1;
	}
	
	if (argc > 2 && *argv[i] == '-')
	{
		sig = atoi(argv[i] + 1);
		i++;
	}
	
	for (; i < argc; i++)
		if (kill(atoi(argv[i]), sig))
		{
			perror(argv[i]);
			err = 1;
		}
	return err;
}
