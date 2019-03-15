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
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

extern char **environ;

void do_help()
{
	printf(
	"minish built-in commands\n\n"
	".ls                 - list files in current directory (.)\n"
	".ls/                - list files in root directory (/)\n"
	".ln OLDNAME NEWNAME - link OLDNAME to NEWNAME\n"
	".mv OLDNAME NEWNAME - link OLDNAME to NEWNAME and unlink OLDNAME\n"
	".rm NAME            - unlink NAME\n"
	".rmdir NAME         - remove directory NAME\n"
	".cd NAME            - change directory to NAME\n"
	".chroot NAME        - change root to NAME (no cwd change occurs)\n"
	".cat NAME           - display contents of file NAME\n"
	".mkdir NAME         - make directory with specified NAME\n"
	".exec NAME ARG ...  - exec NAME with arguments ARG ...\n"
	".exit               - leave minish\n"
	".tty                - show tty name\n"
	".sync               - write cache contents back to disk\n"
	".env                - show environment\n"
	".set ENVSTR         - store ENVSTR in environment: putenv(\"ENVSTR\");\n"
	"NAME ARG ...        - fork and exec NAME with arguments ARG ...\n"
	);
}

void do_ls()
{
	struct dirent *d;
	DIR *dir;
	
	dir = opendir(".");
	if (!dir)
	{
		perror(".");
		return;
	}
	
	for (;;)
	{
		d = readdir(dir);
		if (!d)
		{
			closedir(dir);
			return;
		}
		printf("%10i: %s\n", (int)d->d_ino, d->d_name);
	}
}

void do_lsr()
{
	struct dirent *d;
	DIR *dir;
	
	dir = opendir("/");
	if (!dir)
	{
		perror("/");
		return;
	}
	
	for (;;)
	{
		d = readdir(dir);
		if (!d)
		{
			closedir(dir);
			return;
		}
		printf("%10i: %s\n", (int)d->d_ino, d->d_name);
	}
}

void do_ln(char *p1)
{
	char *p2 = strchr(p1, ' ');
	
	if (!p2)
	{
		fprintf(stderr, "ln: too few arguments\n");
		return;
	}
	
	*p2 = 0;
	p2++;
	
	if (link(p1, p2))
		perror("link");
}

void do_mv(char *p1)
{
	char *p2 = strchr(p1, ' ');
	
	if (!p2)
	{
		fprintf(stderr, "mv: too few arguments\n");
		return;
	}
	
	*p2 = 0;
	p2++;
	
	if (link(p1, p2))
	{
		perror("link");
		return;
	}
	
	if (unlink(p1))
		perror("unlink");
}

void do_rm(char *p)
{
	if (unlink(p))
		perror(p);
}

void do_rmdir(char *p)
{
	if (rmdir(p))
		perror(p);
}

void do_cd(char *p)
{
	if (chdir(p))
		perror(p);
}

void do_chroot(char *p)
{
	if (chroot(p))
		perror(p);
}

void do_cat(char *p)
{
	int fd = open(p, O_RDONLY);
	char c;
	int cnt;
	
	if (fd < 0)
	{
		perror(p);
		return;
	}
	
	while (cnt = read(fd, &c, 1), cnt)
	{
		if (cnt == -1)
		{
			perror(p);
			close(fd);
			return;
		}
		
		write(STDOUT_FILENO, &c, 1);
	}
	
	close(fd);
}

void do_mkdir(char *p)
{
	if (mkdir(p, S_IRWXU | S_IRWXG | S_IRWXO))
		perror(p);
}

void do_extern(char *cmd)
{
	char *arg[16];
	char *p = cmd;
	int status;
	int i = 1;
	
	arg[0] = cmd;
	
	p = strchr(p, ' ');
	if (p)
	{	*p = 0;
		p++;
		
		for (;;)
		{
			char *s = strchr(p,' ');
			int l;
			
			if (i == 15)
			{
				fprintf(stderr, "%s: too many arguments\n", cmd);
				return;
			}
			
			if (s)
				l = s - p;
			else
				l = strlen(p);
			
			arg[i] = p;
			i++;
			if (s)
			{
				*s = 0;
				p = s + 1;
			}
			else
				break;
		}
	}
	
	arg[i] = NULL;
	
	switch (fork())
	{
	case 0:
		execvp(cmd, arg);
		perror(cmd);
		fprintf(stderr, "%s: command not found (try .help)\n", cmd);
		_exit(255);
	case -1:
		perror("fork");
		break;
	}
	
	do
	{
		errno = 0;
		wait(&status);
		if (!errno && WTERMSIG(status))
		{
			if (WCOREDUMP(status))
				printf("Signal %i (core dumped)\n", WTERMSIG(status));
			else
				printf("Signal %i\n", WTERMSIG(status));
		}
	} while(errno != ECHILD);
}

void do_exec(char *cmd)
{
	char *arg[16];
	char *p = cmd;
	int status;
	int i = 1;
	
	arg[0] = cmd;
	
	p = strchr(p, ' ');
	if (p)
	{	*p = 0;
		p++;
		
		for (;;)
		{
			char *s = strchr(p, ' ');
			int l;
			
			if (i == 15)
			{
				fprintf(stderr, "%s: too many arguments\n", cmd);
				return;
			}
			
			if (s)
				l = s - p;
			else
				l = strlen(p);
			
			arg[i] = p;
			i++;
			if (s)
			{
				*s = 0;
				p = s + 1;
			}
			else
				break;
		}
	}
	
	arg[i] = NULL;
	
	execv(cmd, arg);
	perror(cmd);
}

int main()
{
	char buf[256];
	
	for (;;)
	{
		int l;
		
		write(STDOUT_FILENO, "minish> ", 8);
		l = read(STDIN_FILENO, buf, 255);
		
		if (l < 0)
		{
			perror("read");
			exit(0);
		}
		
		buf[l] = 0;
		
		if (!strchr(buf, '\n'))
			exit(0);
		
		*strchr(buf, '\n') = 0;
		
		if (!*buf)
			continue;
		
		if (!strcmp(buf, ".tty"))
		{
			char *n = ttyname(STDIN_FILENO);
			
			if (n)
				printf("%s\n", n);
			else
				printf("not a tty, %m, errno=%i\n", errno);
			continue;
		}
		
		if (!strcmp(buf, ".ls"))
		{
			do_ls();
			continue;
		}
		
		if (!strcmp(buf, ".ls/"))
		{
			do_lsr();
			continue;
		}
		
		if (!strcmp(buf, ".sync"))
		{
			sync();
			continue;
		}
		
		if (!strcmp(buf, ".exit"))
			exit(0);
		
		if (!strncmp(buf, ".ln ",4))
		{
			do_ln(buf + 4);
			continue;
		}
		
		if (!strncmp(buf, ".rm ", 4))
		{
			do_rm(buf + 4);
			continue;
		}
		
		if (!strncmp(buf, ".rmdir ", 7))
		{
			do_rmdir(buf + 7);
			continue;
		}
		
		if (!strncmp(buf, ".cd ", 4))
		{
			do_cd(buf + 4);
			continue;
		}
		
		if (!strncmp(buf, ".chroot ", 8))
		{
			do_chroot(buf + 8);
			continue;
		}
		
		if (!strncmp(buf, ".mv ", 4))
		{
			do_mv(buf + 4);
			continue;
		}
		
		if (!strncmp(buf, ".cat ", 5))
		{
			do_cat(buf + 5);
			continue;
		}
		
		if (!strncmp(buf, ".mkdir ", 7))
		{
			do_mkdir(buf + 7);
			continue;
		}
		
		if (!strncmp(buf, ".exec ", 6))
		{
			do_exec(buf + 6);
			continue;
		}
		
		if (!strcmp(buf, ".env"))
		{
			int i = 0;
			
			while (environ[i])
				printf("%s\n", environ[i++]);
			continue;
		}
		
		if (!strncmp(buf, ".set ", 5))
		{
			if (putenv(strdup(buf + 5)))
				perror("putenv");
			continue;
		}
		
		if (!strcmp(buf, ".help"))
		{
			do_help();
			continue;
		}
		
		do_extern(buf);
	}
	
	return 0;
}
