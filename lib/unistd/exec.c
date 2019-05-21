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

#include <environ.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int _exec(char *name, char *arg, char *env);

int execve(char *path, char **argv, char **envp)
{
	char arg[ARG_MAX];
	char env[ARG_MAX];
	struct stat st;
	int cnt;
	
	*arg = 0;
	*env = 0;
	
	while (*argv)
	{
		if (strlen(*argv) + strlen(arg) + 1 >= ARG_MAX)
		{
			errno = E2BIG;
			return -1;
		}
		if (strchr(*argv, '\377'))
		{
			errno = EINVAL;
			return -1;
		}
		strcat(arg, *argv);
		strcat(arg, "\377");
		argv++;
	}
	if (envp)
		while (*envp)
		{
			if (strlen(*envp) + strlen(env) + 1 >= ARG_MAX)
			{
				errno = E2BIG;
				return -1;
			}
			if (strchr(*envp, '\377'))
			{
				errno = EINVAL;
				return -1;
			}
			strcat(env, *envp);
			strcat(env, "\377");
			envp++;
		}
	
	return _exec(path, arg, env);
}

int execv(char *path, char **argv)
{
	return execve(path, argv, environ);
}

int execl(char *path, ...)
{
	return execv(path, &path + 1);
}

int execle(char *path, ...)
{
	char ***envpp = (char ***)&path + 1;
	
	while (*(envpp++));
	return execve(path, &path + 1, *envpp);
}

int execvp(char *path, char **argv)
{
	char exe[PATH_MAX];
	struct stat st;
	int err = ENOENT;
	char *p = getenv("PATH");
	int pl = strlen(path);
	char *s;
	int l;
	
	if (strchr(path, '/'))
		return execv(path, argv);
	
	if (!p)
		p = ":/bin:/usr/bin";
	
	while (p)
	{
		s = strchr(p, ':');
		if (s)
			l = s - p;
		else
			l = strlen(p);
		
		if (l + pl + 2 < PATH_MAX)
		{
			memcpy(exe, p, l);
			if (l)
			{
				exe[l] = '/';
				l++;
			}
			strcpy(exe + l, path);
			
			if (stat(exe, &st))
			{
				if (errno != ENOENT)
					err = errno;
				goto next;
			}
			
			if (!access(exe, X_OK))
				return execv(exe, argv);
		}
next:
		if (!s)
			break;
		p = s + 1;
	}
	
	errno = err;
	return -1;
}

int execlp(char *path, ...)
{
	return execvp(path, &path + 1);
}
