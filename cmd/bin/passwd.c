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

#include <xenus/uids.h>
#include <sys/termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <md5.h>
#include <pwd.h>

#define TPWD	"/etc/passwd.tmp"
#define TSPWD	"/etc/spwd.tmp"

static struct termios stio;

static char *nshell;
static char *npass;

void setpass(uid_t uid)
{
	struct passwd *pw;
	FILE *f1;
	FILE *f2;
	int fd1;
	int fd2;
	
restart:
	fd1 = open(TSPWD, O_CREAT | O_EXCL | O_WRONLY, 0600);
	if (fd1 < 0)
	{
		if (errno == EEXIST)
		{
			sleep(1);
			goto restart;
		}
		perror(TSPWD);
		exit(errno);
	}
	fd2 = open(TPWD, O_CREAT | O_EXCL | O_WRONLY, 0644);
	if (fd2 < 0)
	{
		perror(TPWD);
		unlink(TSPWD);
		exit(errno);
	}
	
	f1 = fdopen(fd1, "w");
	if (!f1)
	{
		perror(TSPWD);
		unlink(TSPWD);
		unlink(TPWD);
		exit(errno);
	}
	
	f2 = fdopen(fd2, "w");
	if (!f1)
	{
		perror(TPWD);
		unlink(TSPWD);
		unlink(TPWD);
		exit(errno);
	}
	
	setpwent();
	errno = 0;
	while (pw = getpwent(), pw)
	{
		if (pw->pw_uid == uid)
		{
			if (nshell)
				pw->pw_shell = nshell;
			if (npass)
				pw->pw_passwd = npass;
		}
		if (putpwent(pw, f1))
		{
			perror(TSPWD);
			unlink(TSPWD);
			unlink(TPWD);
			exit(errno);
		}
		if (*pw->pw_passwd)
			pw->pw_passwd = "x";
		if (putpwent(pw, f2))
		{
			perror(TPWD);
			unlink(TSPWD);
			unlink(TPWD);
			exit(errno);
		}
	}
	fclose(f1);
	fclose(f2);
	if (rename(TSPWD, "/etc/spwd"))
	{
		perror(TSPWD "-> /etc/spwd");
		unlink(TSPWD);
		unlink(TPWD);
		exit(errno);
	}
	if (rename(TPWD, "/etc/passwd"))
	{
		perror(TPWD "-> /etc/passwd");
		unlink(TSPWD);
		unlink(TPWD);
		exit(errno);
	}
}

void intr(int nr)
{
	tcsetattr(0, TCSANOW, &stio);
	raise(nr);
}

static int isshell(char *path)
{
	struct stat st;
	
	if (!*path)
		return 1;
	
	if (!getuid())
		return 1;
	
	if (access(path, X_OK))
		return 0;
	
	if (stat(path, &st))
		return 0;
	
	if (!S_ISREG(st.st_mode))
		return 0;
	
	return 1;
}

int main(int argc, char **argv)
{
	char passwd1[MAX_CANON + 1];
	char passwd2[MAX_CANON + 1];
	struct passwd *pw;
	uid_t ruid;
	char *cmd = "passwd";
	char *p;
	int i;
	
	if (argc)
	{
		cmd = strrchr(argv[0], '/');
		if (!cmd)
			cmd = argv[0];
		else
			cmd++;
	}
	
	ruid = getuid();
	umask(0);
	
	for (i = 0; i < NSIG; i++)
		signal(i, SIG_IGN);
	signal(SIGINT, SIG_DFL);
	
	for (i = 0; i < OPEN_MAX; i++)
		close(i);
	
	if (open("/dev/tty", O_RDONLY) != STDIN_FILENO)
		return 1;
	if (open("/dev/tty", O_WRONLY) != STDOUT_FILENO)
		return 1;
	if (open("/dev/tty", O_WRONLY) != STDERR_FILENO)
		return 1;
	
	if (ruid && argc > 1)
	{
		fputs("Not permitted\n", stderr);
		return 1;
	}
	
	if (argc > 2)
	{
		fputs("passwd [user]\n", stderr);
		return 1;
	}
	
	if (ruid == UGUEST)
	{
		fputs("Not permitted\n", stderr);
		return 1;
	}
	
	errno = 0;
	if (argc == 2)
		pw = getpwnam(argv[1]);
	else
		pw = getpwuid(getuid());
	
	if (!pw)
	{
		if (!errno)
		{
			fputs("Account not found\n", stderr);
			return 1;
		}
		
		perror("/etc/spwd");
		return 1;
	}
	
	tcgetattr(0, &stio);
	signal(SIGINT, intr);
	
	if (*pw->pw_passwd && ruid)
	{
		if (!strcmp(cmd, "passwd"))
			printf("Old password: ");
		else
			printf("Password: ");
		fflush(stdout);
		getpassn(passwd1, sizeof(passwd1));
		putchar('\n');
		
		if (strcmp(pw->pw_passwd, md5a(passwd1)))
		{
			for (i = 1; i < NSIG; i++)
				signal(i, SIG_IGN);
			sleep(3);
			fputs("Password incorrect\n", stderr);
			sleep(2);
			return 1;
		}
	}
	
	if (!strcmp(cmd, "chsh"))
	{
		printf("New shell: ");
		fflush(stdout);
		
		if (!fgets(passwd1, sizeof passwd1, stdin))
		{
			if (ferror(stdin))
				perror(NULL);
			return 1;
		}
		
		p = strchr(passwd1, '\n');
		if (p)
			*p = 0;
		
		if (!isshell(passwd1))
		{
			fputs("Invalid shell\n", stderr);
			return 1;
		}
		
		nshell = passwd1;
	}
	else
	{
		printf("New password: ");
		fflush(stdout);
		getpassn(passwd1, sizeof(passwd1));
		printf("\nRetype: ");
		fflush(stdout);
		getpassn(passwd2, sizeof(passwd2));
		printf("\n");
		fflush(stdout);
		
		if (strcmp(passwd1, passwd2))
		{
			fputs("Passwords do not match\n", stderr);
			return 1;
		}
		
		if (strlen(passwd1) > PASSWD_MAX)
		{
			fputs("Password too long\n", stderr);
			return 1;
		}
		
		npass = *passwd1 ? md5a(passwd1) : "";
	}
	
	signal(SIGINT, SIG_IGN);
	setpass(pw->pw_uid);
	return 0;
}
