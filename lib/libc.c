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

#include <sys/types.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

extern int _end;
extern int _edata;

static char **argv;
static int argc;

char **environ;
char ***_envp = &environ;

char __libc_panic_msg[]="c ";

int main(int argc, char **argv, char **envp);

void __libc_stdio_init(void);

void __libc_panic(char *desc)
{
	write(STDERR_FILENO, __libc_panic_msg, sizeof(__libc_panic_msg) - 1);
	write(STDERR_FILENO, desc, strlen(desc));
	write(STDERR_FILENO, "\n", 1);
	_exit(255);
}

void __libc_init()
{
	__libc_stdio_init();
}

static void safeio(void)
{
	int i;
	
	if (getuid() == geteuid() && getgid() == getegid())
		return;
	
	for (i = 0; i <= 2; i++)
		if (fcntl(i, F_GETFL) < 0)
			_exit(255);
}

static void oom(void)
{
	__libc_panic("oom");
}

static void split(char *s, char **v)
{
	char *p;
	
	for (p = s; *s; s++)
		if (*s == '\377')
		{
			*v++ = p;
			p = s + 1;
			*s = 0;
		}
}

static int count(char *s)
{
	int cnt;
	
	for (cnt = 0; *s; s++)
		if (*s == '\377')
			cnt++;
	return cnt;
}

static void initargenv(char *argenv)
{
	char *env;
	char *p;
	int envc;
	int i;
	
	env = argenv + strlen(argenv) + 1;
	
	argc = count(argenv);
	envc = count(env);
	
	environ	= sbrk((envc + 1) * sizeof(char *));
	argv	= sbrk((argc + 1) * sizeof(char *));
	
	if (!argv || !environ)
		oom();
	
	split(argenv, argv);
	split(env, environ);
}

void __libc_start(char *argenv)
{
	safeio();
	__libc_init();
	initargenv(argenv);
	errno = 0;
	exit(main(argc, argv, environ));
	for (;;);
}
