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
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

static char *snames[] = { "c", "f" };
static char *sect;
static char *moreflg = "-bx";
static int iflag;
static int xit;

static int help1(char *sname, char *name)
{
	char path[PATH_MAX];
	char buf[512];
	pid_t pid;
	int cnt;
	int fd;
	
	sprintf(path, "/usr/man/cat%s/%s", sname, name);
	if (access(path, R_OK))
		return 0;
	
	pid = fork();
	if (pid < 0)
	{
		perror(NULL);
		return 0;
	}
	if (!pid)
	{
		execl("/bin/more", "/bin/more", moreflg, path, (void *)NULL);
		perror("/bin/more");
		exit(1);
	}
	while (wait(NULL) != pid)
		;
	return 1;
}

static void help(char *name)
{
	int i;
	
	if (sect)
	{
		if (!help1(sect, name))
			goto fail;
		return;
	}
	
	for (i = 0; i < sizeof snames / sizeof *snames; i++)
		if (help1(snames[i], name))
			return;
fail:
	fprintf(stderr, "%s not found\n", name);
}

static void showidx1(char *sect)
{
	char path[PATH_MAX + 1];
	pid_t pid;
	
	sprintf(path, "/usr/man/cat%s", sect);
	printf("\nSection '%s'\n\n", sect);
	
	pid = fork();
	if (pid < 0)
	{
		perror(NULL);
		xit = 1;
		return;
	}
	if (!pid)
	{
		execlp("lc", "lc", path, (void *)NULL);
		perror("lc");
		exit(1);
	}
	while (wait(NULL) != pid)
		;
}

static void showidx(void)
{
	int i;
	
	if (sect)
	{
		showidx1(sect);
		return;
	}
	
	for (i = 0; i < sizeof snames / sizeof *snames; i++)
		showidx1(snames[i]);
}

int main(int argc, char **argv)
{
	char *mancolor;
	int i;
	
	mancolor = getenv("MANCOLOR");
	if (mancolor)
	{
		switch (atoi(mancolor))
		{
		case 0:
			moreflg = "-bx";
			break;
		case 1:
			moreflg = "-b";
			break;
		default:
			moreflg = "-bc";
		}
	}
	
	if (argc > 1 && !strcmp(argv[1], "-i"))
	{
		iflag = 1;
		argv++;
		argc--;
	}
	
	if (argc > 1)
		for (i = 0; i < sizeof snames / sizeof *snames; i++)
			if (!strcmp(argv[1], snames[i]))
			{
				sect = argv[1];
				argv++;
				argc--;
				break;
			}
	
	if (iflag)
	{
		showidx();
		return 0;
	}
	
	if (argc < 2)
	{
		fputs("man [section] name1...\n", stderr);
		fputs("man -i [section]\n", stderr);
		return 1;
	}
	
	signal(SIGCHLD, SIG_DFL);
	
	for (i = 1; i < argc; i++)
		help(argv[i]);
	return 0;
}
