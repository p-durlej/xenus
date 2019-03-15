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

#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#include <stdio.h>
#include <errno.h>

int system(char *cmd)
{
	void *sigchldh;
	void *sigquith;
	void *siginth;
	pid_t pid;
	int ret;
	
	sigchldh = signal(SIGCHLD, SIG_IGN);
	sigquith = signal(SIGQUIT, SIG_IGN);
	siginth	 = signal(SIGINT, SIG_IGN);
	
	pid = fork();
	if (!pid)
	{
		signal(SIGQUIT, sigquith);
		signal(SIGINT, siginth);
		execl("/bin/sh", "/bin/sh", "-c", cmd, (void *)0);
		_exit(127);
	}
	
	if (pid > 0 && wait(&ret) < 0) // XXX
		ret = -1;
	if (pid < 0)
		ret = -1;
	
	signal(SIGCHLD, sigchldh);
	signal(SIGQUIT, sigquith);
	signal(SIGINT, siginth);
	return ret;
}
