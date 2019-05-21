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

#include <limits.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

static char	pathbuf[PATH_MAX];

static char **	n_argv;
static int	n_argc;

static void addarg(char *arg);
static void aname(char *name);
static int  match(char *filename, char *pat, size_t plen);
static int  glob1(char *pat, size_t plen);
static int  globs(char *arg);
static void globa(char *arg);

static void toolong(void)
{
	errno = ENAMETOOLONG;
	perror(NULL);
	exit(1);
}

static void addarg(char *arg)
{
	n_argv = realloc(n_argv, sizeof *n_argv * (n_argc + 2));
	if (n_argv == NULL)
	{
		perror(NULL);
		exit(1);
	}
	n_argv[n_argc++] = (char *)arg;
	n_argv[n_argc  ] = NULL;
}

static void aname(char *name)
{
	size_t dir_len;
	size_t nam_len;
	size_t tot_len;
	
	dir_len = strlen(pathbuf);
	nam_len = strlen(name);
	tot_len = dir_len + nam_len;
	
	if (dir_len && pathbuf[dir_len - 1] != '/')
		tot_len++;
	
	if (tot_len >= sizeof pathbuf)
		toolong();
	
	if (dir_len && pathbuf[dir_len - 1] != '/')
		strcat(pathbuf, "/");
	strcat(pathbuf, name);
}

static int match(char *filename, char *pat, size_t plen)
{
	char *p;
	
	if (!strcmp(filename, ".") || !strcmp(filename, ".."))
		return 0;
	
	while (plen)
	{
		switch (*pat)
		{
		case '*':
			while (plen && *pat == '*')
			{
				plen--;
				pat++;
			}
			if (!plen)
				return 1;
			for (p = filename; *p; p++)
				if (match(p, pat, plen))
					return 1;
			continue;
		case '?':
			if (!*filename)
				return 0;
			break;
		default:
			if (*pat != *filename)
				return 0;
		}
		filename++;
		pat++;
		plen--;
	}
	if (*filename)
		return 0;
	return 1;
}

static int glob1(char *pat, size_t plen)
{
	size_t pfx_len;
	struct dirent *de;
	char *odn;
	char *p;
	int m = 0;
	DIR *d;
	
	pfx_len = strlen(pathbuf);
	
	while (*pat == '/')
	{
		plen--;
		pat++;
	}
	
	odn = pathbuf;
	if (!*odn)
		odn = ".";
	
	d = opendir(odn);
	if (d == NULL)
		return 0; /* XXX error */
	while (errno = 0, de = readdir(d), de != NULL)
		if (match(de->d_name, pat, plen))
		{
			aname(de->d_name);
			p = pat + plen;
			if (globs(p))
				m = 1;
			pathbuf[pfx_len] = 0;
		}
	closedir(d);
	return m;
}

static int globs(char *arg)
{
	char *dir_s, *dir_e, *pat_s, *pat_e;
	size_t pfx_len;
	size_t dir_len;
	size_t pat_len;
	char *p;
	int m;
	
	pfx_len = strlen(pathbuf);
	for (dir_s = arg; *dir_s == '/'; dir_s++);
	dir_e = dir_s + strcspn(dir_s, "*?[");
	if (!*dir_e)
	{
		if (pfx_len + strlen(arg) >= sizeof pathbuf)
			toolong();
		strcat(pathbuf, arg);
		if (!access(pathbuf, 0))
		{
			p = strdup(pathbuf);
			if (p == NULL)
			{
				perror(NULL);
				exit(1);
			}
			addarg(p);
			return 1;
		}
		pathbuf[pfx_len] = 0;
		return 0;
	}
	while (dir_e > dir_s && *dir_e != '/')
		dir_e--;
	dir_len = dir_e - dir_s;
	
	for (pat_s = dir_e; *pat_s == '/'; pat_s++);
	for (pat_e = pat_s; *pat_e != '/' && *pat_e; pat_e++);
	pat_len = pat_e - pat_s;
	
	if (pfx_len + pat_len >= sizeof pathbuf)
		toolong();
	memcpy(pathbuf + pfx_len, dir_s, dir_len);
	pathbuf[pfx_len + dir_len] = 0;
	
	m = glob1(pat_s, pat_e - pat_s);
	pathbuf[pfx_len] = 0;
	return m;
}

static int argcmp(void *p1, void *p2)
{
	char *n1 = *(char **)p1;
	char *n2 = *(char **)p2;
	int r;
	
	r = strcmp(n1, n2);
	if (r > 0)
		return 1;
	if (r < 0)
		return -1;
	return 0;
}

static void globa(char *arg)
{
	int oargc = n_argc;
	int i;
	
	*pathbuf = 0;
	if (*arg == '/')
		strcpy(pathbuf, "/");
	if (!globs(arg))
		aname(arg);
	qsort(n_argv + oargc, n_argc - oargc, sizeof *n_argv, argcmp);
}

#ifdef SHELL
int globx(int Xarg, char **argv)
#else
int main(int argc, char **argv)
#endif
{
	char *p;
	int i;
	
#ifndef SHELL
	if (argc < 2)
	{
		fputs("wrong nr of args\n", stderr);
		return 1;
	}
#endif
	addarg(argv[1]);
	
	for (i = 2; argv[i]; i++)
	{
		p = argv[i] + strcspn(argv[i], "*?");
		if (*p)
			globa(argv[i]);
		else
			addarg(argv[i]);
	}
	execvp(argv[1], n_argv);
	if (errno == ENOENT)
		fprintf(stderr, "%s: Command not found\n", argv[1]);
	else
		perror(argv[1]);
	exit(127);
	return 127; /* XXX silence a warning */
}
