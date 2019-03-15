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
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static char path[PATH_MAX];
static char **expr;
static int xit;

static int otoi(char *nptr)
{
	int n = 0;
	
	while (*nptr >= '0' && *nptr <= '7')
	{
		n <<= 3;
		n  += *nptr++ - '0';
	}
	return n;
}

static int nmatch(char *pat, char *name)
{
	char *p1, *p2;
	
	for (p1 = pat, p2 = name; *p1 && *p2; p1++)
		switch (*p1)
		{
		case '?':
			if (!*p2++)
				return 0;
			break;
		case '*':
			while (*p2)
				if (nmatch(p1 + 1, p2++))
					return 1;
			break;
		default:
			if (*p2++ != *p1)
				return 0;
		}
	if (*p1 || *p2)
		return 0;
	return 1;
}

static void eval(struct stat *st, char *name)
{
	char **p;
	
	for (p = expr; *p; p++)
	{
		if (!strcmp(*p, "-print"))
		{
			puts(path);
			continue;
		}
		
		if (!strcmp(*p, "-type"))
		{
			mode_t t;
			
			switch (**++p)
			{
			case 'f': t = S_IFREG; break;
			case 'd': t = S_IFDIR; break;
			case 'c': t = S_IFCHR; break;
			case 'b': t = S_IFBLK; break;
			case 'p': t = S_IFIFO; break;
			default:
				fprintf(stderr, "Bad inode type '%c'\n", **p);
				exit(1);
			}
			
			if ((st->st_mode & S_IFMT) != t)
				return;
			continue;
		}
		
		if (!strcmp(*p, "-name"))
		{
			if (!nmatch(*++p, name))
				return;
			continue;
		}
		
		if (!strcmp(*p, "-perm"))
		{
			mode_t m, mask = 07777;
			
			if (**++p == '-')
				m = mask = otoi(*p + 1);
			else
				m = otoi(*p);
			
			if ((st->st_mode & mask) != m)
				return;
			continue;
		}
		
		if (!strcmp(*p, "-inum"))
		{
			if (st->st_ino != atoi(*++p))
				return;
			continue;
		}
		
		if (!strcmp(*p, "-exec"))
		{
			char **sp, **np = NULL;
			pid_t pid;
			
			for (sp = ++p; *p; p++)
			{
				if (!strcmp(*p, "{}"))
					np = p;
				if (!strcmp(*p, ";"))
					break;
			}
			
			if (p - sp < 1)
			{
				fputs("Missing operand\n", stderr);
				exit(1);
			}
			
			if (!*p)
			{
				fputs("No terminating ';'\n", stderr);
				exit(1);
			}
			
			pid = fork();
			if (!pid)
			{
				if (np)
					*np = path;
				*p = NULL;
				
				execvp(*sp, sp);
				perror(*sp);
				exit(127);
			}
			if (pid < 0)
			{
				perror("fork");
				xit = 1;
				continue;
			}
			while (wait(NULL) != pid)
				;
			continue;
		}
		
		fprintf(stderr, "Unknown option '%s'\n", *p);
		exit(1);
	}
}

static void find1(void)
{
	struct dirent *de;
	struct stat st;
	char *n;
	DIR *d;
	int l;
	
	if (!*path)
	{
		fputs("Empty path", stderr);
		xit = 1;
		return;
	}
	
	d = opendir(path);
	if (!d)
	{
		perror(path);
		xit = 1;
		return;
	}
	
	l = strlen(path);
	if (path[l - 1] == '/')
		n = path + l;
	else
	{
		n = path + l + 1;
		path[l] = '/';
	}
	
	while (de = readdir(d), de)
	{
		if (!strcmp(de->d_name, "."))
			continue;
		if (!strcmp(de->d_name, ".."))
			continue;
		strcpy(n, de->d_name);
		
		if (stat(path, &st))
		{
			perror(path);
			xit = 1;
			continue;
		}
		eval(&st, n);
		
		if (S_ISDIR(st.st_mode))
			find1();
	}
	closedir(d);
	path[l] = 0;
}

int main(int argc, char **argv)
{
	int pc;
	int i;
	
	if (argc < 2)
	{
		fputs("find path1... predicate1...\n", stderr);
		return 1;
	}
	
	for (pc = 1; pc < argc; pc++)
		if (*argv[pc] == '-')
			break;
	expr = argv + pc;
	
	for (i = 1; i < pc; i++)
	{
		strcpy(path, argv[i]);
		find1();
	}
	return xit;
}
