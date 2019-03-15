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
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <utmp.h>
#include <time.h>
#include <pwd.h>
#include <md5.h>

int _killf(char *path, int sig);

char passwd[MAX_CANON + 1];
char user[MAX_CANON + 1];

char termvar[32] = "TERM=XXX";

char *nenv[] =
{
	"PATH=:/bin:/usr/bin",
	"HOME=/",
	NULL, /* TERM */
	NULL
};

void incorrect()
{
	sleep(3);
	printf("Login incorrect.\n");
	sleep(2);
	exit(255);
}

void sigign()
{
	int i;
	
	for (i = 0;i < NSIG; i++)
		signal(i, SIG_IGN);
	signal(SIGALRM, SIG_DFL);
	signal(SIGHUP, SIG_DFL);
}

void sigdfl()
{
	int i;
	
	for (i = 0; i < NSIG; i++)
		signal(i, SIG_DFL);
}

void tty_reset()
{
	char *name;
	
	name = ttyname(STDIN_FILENO);
	if (!name)
	{
		perror("ttyname");
		incorrect();
	}
	_killf(name, SIGKILL);
}

void tty_chown(uid_t uid)
{
	char *name;
	
	name = ttyname(STDIN_FILENO);
	if (!name)
	{
		perror("ttyname");
		incorrect();
	}
	if (chown(name, uid, GTTY))
		goto fail;
	if (chmod(name, S_IRUSR | S_IWUSR))
		goto fail;
	return;
fail:
	perror(name);
	incorrect();
}

void sutmp(void)
{
	struct utmp ut;
	char *name;
	int fd;
	
	name = ttyname(STDIN_FILENO);
	if (!name)
		return;
	name = strrchr(name, '/') + 1;
	
	fd = open("/etc/utmp", O_RDWR);
	if (fd < 0)
	{
		if (errno != ENOENT && errno != EROFS)
			perror("/etc/utmp");
		return;
	}
	
	while (read(fd, &ut, sizeof ut) == sizeof ut)
	{
		if (strncmp(name, ut.ut_line, sizeof ut.ut_line))
			continue;
		
		strncpy(ut.ut_name, user, sizeof ut.ut_name);
		time(&ut.ut_time);
		lseek(fd, -sizeof ut, SEEK_CUR);
		write(fd, &ut, sizeof ut);
		close(fd);
		return;
	}
	fprintf(stderr, "%s not in /etc/utmp\n", name);
	close(fd);
}

void show(char *name)
{
	int fd = open(name, O_RDONLY);
	char buf[512];
	int cnt;
	
	if (fd < 0)
	{
		perror(name);
		return;
	}
	
	while (cnt = read(fd, buf, sizeof(buf)), cnt)
	{
		if (cnt < 0)
		{
			perror(name);
			close(fd);
			return;
		}
		if (write(STDOUT_FILENO, buf, cnt) != cnt)
		{
			perror("stdout");
			close(fd);
			return;
		}
	}
	
	close(fd);
}

void input(char *s, int l)
{
	int cnt;
	
	cnt = read(STDIN_FILENO, s, l - 1);
	if (cnt < 0)
	{
		perror("stdin");
		incorrect();
	}
	
	s[cnt] = 0;
	if (strchr(s, '\n'))
		*strchr(s, '\n') = 0;
}

void sttype(void)
{
	char buf[96];
	char *p, *n;
	char *ttn;
	FILE *f;
	
	ttn = ttyname(0);
	if (!ttn)
		return;
	
	ttn = strrchr(ttn, '/') + 1;
	if (ttn == (void *)1)
		return;
	
	f = fopen("/etc/ttytype", "r");
	if (!f)
		return;
	
	while (fgets(buf, sizeof buf, f))
	{
		p = strchr(buf, '\n');
		if (p)
			*p = 0;
		
		p = strtok(buf,  " \t");
		n = strtok(NULL, " \t");
		
		if (p && n && !strcmp(n, ttn))
		{
			nenv[2] = sbrk(strlen(p) + 6);
			strcpy(nenv[2], "TERM=");
			strcat(nenv[2], p);
			break;
		}
	}
	fclose(f);
}

void login(char *user, char *passwd)
{
	struct passwd *pw;
	char *sh;
	char *p;
	
	pw = getpwnam(user);
	if (!pw)
		incorrect();
	
	if (*pw->pw_passwd && strcmp(pw->pw_passwd, md5a(passwd)))
		incorrect();
	
	sutmp();
	
	tty_chown(pw->pw_uid);
	
	p = sbrk(strlen(pw->pw_dir) + 6);
	strcpy(p, "HOME=");
	strcat(p, pw->pw_dir);
	nenv[1] = p;
	
	if (!pw->pw_uid)
		nenv[0] = "PATH=/bin:/usr/bin";
	
	setgid(pw->pw_gid);
	setuid(pw->pw_uid);
	endpwent();
	chdir(pw->pw_dir);
	umask(022);
	
	if (!access("/etc/motd", 0))
		show("/etc/motd");
	
	sh = *pw->pw_shell ? pw->pw_shell : "/bin/sh";
	
	sigdfl();
	sttype();
	execle(sh, "-", NULL, nenv);
	perror(sh);
	sigign();
	incorrect();
}

void prompt(void)
{
	struct passwd *pw;
	
	do
	{
		write(STDOUT_FILENO, "\nlogin: ", 8);
		input(user, sizeof(user));
	} while (!*user);
	
	pw = getpwnam(user);
	if (!pw || *pw->pw_passwd)
	{
		write(STDOUT_FILENO, "Password: ", 10);
		getpassn(passwd, sizeof(passwd));
		write(STDOUT_FILENO, "\n", 1);
	}
}

int main(int argc, char **argv)
{
	int i;
	
	if (geteuid())
	{
		fputs("Not permitted\n", stderr);
		return 1;
	}
	
	sigign();
	
	tty_chown(0);
	tty_reset();
	
	if (!access("/etc/issue", 0))
		show("/etc/issue");
	
	prompt();
	
	if (!access("/etc/nologin", 0))
	{
		show("/etc/nologin");
		if (strcmp(user, "root"))
			incorrect();
	}
	
	login(user, passwd);
}
