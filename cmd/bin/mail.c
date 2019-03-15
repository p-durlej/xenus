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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <pwd.h>

#define SPOOLDIR "/usr/spool/mail/"
#define MAXMSG	 1024

static char mailbox[PATH_MAX];
static long msgoff[MAXMSG + 1];
static int msgdel[MAXMSG];
static int msgcnt;
static int msgmod;
static FILE *mailboxf;
static int xit;
static mode_t um;
static char *user;
static int fflag;

static ssize_t xwrite(int fd, void *buf, size_t cnt)
{
	ssize_t rcnt;
	
	rcnt = write(fd, buf, cnt);
	if (rcnt < 0)
	{
		perror(NULL);
		xit = 1;
		return -1;
	}
	if (rcnt < cnt)
	{
		fputs("Short write", stderr);
		xit = 1;
		return -1;
	}
	return 0;
}

static int lock(char *name)
{
	char path[PATH_MAX + 1];
	int fd;
	
	if (fflag)
		return 0;
	
	strcpy(path, SPOOLDIR);
	strcat(path, name);
	strcat(path, ".lck");
retry:
	fd = open(path, O_CREAT | O_EXCL | O_WRONLY, 0666);
	if (fd < 0)
	{
		if (errno == EEXIST)
		{
			sleep(1);
			goto retry;
		}
		perror(path);
		return -1;
	}
	close(fd);
	return 0;
}

static void unlock(char *name)
{
	char path[PATH_MAX + 1];
	
	if (fflag)
		return;
	
	strcpy(path, SPOOLDIR);
	strcat(path, name);
	strcat(path, ".lck");
	
	if (unlink(path))
		perror(path);
}

static void send1(int fd, char *addr)
{
	static char buf[512];
	
	char path[PATH_MAX];
	struct passwd *pw;
	ssize_t rcnt, wcnt;
	int ofd;
	
	pw = getpwnam(addr);
	if (!pw)
	{
		fprintf(stderr, "User %s not found\n", addr);
		xit = 1;
		return;
	}
	
	if (lock(addr))
		return;
	
	strcpy(path, SPOOLDIR);
	strcat(path, addr);
	
	lseek(fd, 0, SEEK_SET);
	
	ofd = open(path, O_WRONLY | O_APPEND);
	if (ofd < 0 && errno == ENOENT)
	{
		ofd = open(path, O_CREAT | O_EXCL | O_WRONLY | O_APPEND, 0600);
		if (ofd >= 0)
			fchown(ofd, pw->pw_uid, pw->pw_gid);
	}
	if (ofd < 0)
	{
		perror(path);
		unlock(addr);
		xit = 1;
		return;
	}
	
	while (rcnt = read(fd, buf, sizeof buf), rcnt > 0)
		if (xwrite(ofd, buf, rcnt))
			break;
	if (rcnt < 0)
	{
		perror(NULL);
		xit = 1;
	}
	
	close(ofd);
	unlock(addr);
}

static void send(char **addrs)
{
	struct passwd *pw;
	char buf[100];
	pid_t pid;
	time_t t;
	int rcnt;
	int len;
	int fd;
	int i;
	
	pid = getpid();
	for (i = 0; i < 100; i++)
	{
		sprintf(buf, "/tmp/mail%02i%04i", i, pid);
		fd = open(buf, O_CREAT | O_EXCL | O_RDWR, 0600);
		if (fd < 0)
		{
			if (errno == EEXIST)
				continue;
			perror(buf);
			xit = 1;
			return;
		}
		unlink(buf);
		break;
	}
	
	time(&t);
	sprintf(buf, "From %s %s", user, ctime(&t));
	if (xwrite(fd, buf, strlen(buf)))
		goto fail;
	
	while (fgets(buf, sizeof buf, stdin))
	{
		if (!strcmp(buf, ".") || !strcmp(buf, ".\n"))
			break;
		if (!strncmp(buf, "From ", 5))
			if (xwrite(fd, "> ", 2))
				goto fail;
		len = strlen(buf);
		if (xwrite(fd, buf, len))
			goto fail;
	}
	xwrite(fd, "\n", 1);
	
	for (i = 0; addrs[i]; i++)
		send1(fd, addrs[i]);
	
fail:
	close(fd);
}

static void load(void)
{
	char buf[100];
	long off = 0;
	long off1;
	FILE *f;
	
	f = fopen(mailbox, "r");
	if (!f)
		return;
	
	while (off1 = ftell(f), fgets(buf, sizeof buf, f))
	{
		if (!strncmp(buf, "From ", 5))
		{
			if (!off1)
			{
				off = off1;
				continue;
			}
			if (msgcnt >= MAXMSG)
			{
				fputs("Too many messages", stderr);
				break;
			}
			msgoff[msgcnt++] = off;
			off = off1;
		}
	}
	if (off1)
	{
		msgoff[msgcnt++] = off;
		msgoff[msgcnt  ] = ftell(f);
	}
	mailboxf = f;
}

static void print(int i)
{
	long off, end;
	int c;
	
	off = msgoff[i];
	end = msgoff[i + 1];
	
	fseek(mailboxf, off, SEEK_SET);
	while (off++ < end)
	{
		c = fgetc(mailboxf);
		if (c == EOF)
			break;
		putchar(c);
	}
}

static void update(void)
{
	char path[PATH_MAX];
	FILE *f, *f2 = NULL;
	long cnt;
	int c;
	int i;
	
	if (lock(user))
		return;
	
	strcpy(path, mailbox);
	strcat(path, ".tmp");
	
	f = fopen(path, "w+");
	if (!f)
		goto fail;
	
	for (i = 0; i < msgcnt; i++)
	{
		if (msgdel[i])
			continue;
		
		fseek(mailboxf, msgoff[i], SEEK_SET);
		cnt = msgoff[i + 1] - msgoff[i];
		
		while (cnt--)
		{
			c = fgetc(mailboxf);
			if (c == EOF)
				goto fail;
			if (fputc(c, f) == EOF)
				goto fail;
		}
	}
	
	fseek(mailboxf, msgoff[msgcnt], SEEK_SET);
	while (c = fgetc(mailboxf), c != EOF)
		fputc(c, f);
	
	rewind(f);
	
	f2 = fopen(mailbox, "w");
	if (!f2)
		goto fail;
	
	while (c = fgetc(f), c != EOF)
		if (fputc(c, f2) == EOF)
			goto fail;
	if (fclose(f2))
		goto fail;
	if (fclose(f))
		goto fail;
	
	unlink(path);
	unlock(user);
	return;
fail:
	perror(NULL);
	xit = 1;
	if (f2)
		fclose(f2);
	if (f)
		fclose(f);
	unlock(user);
}

static int save(int i, char *path)
{
	struct stat st1, st2;
	char pbuf[PATH_MAX];
	FILE *f, *oo;
	pid_t pid;
	char *p;
	int st;
	
	pid = fork();
	if (!pid)
	{
		setgid(getgid());
		setuid(getuid());
		
		p = strchr(path, '\n');
		if (p)
			*p = 0;
		
		while (isspace(*path))
			path++;
		
		if (!*path)
		{
			path = getenv("HOME");
			if (!path)
			{
				fputs("No home\n", stderr);
				exit(1);
			}
			strcpy(pbuf, path);
			strcat(pbuf, "/mbox");
			path = pbuf;
		}
		
		umask(um);
		
		f = fopen(path, "a");
		if (!f)
			goto fail;
		
		fstat(fileno(mailboxf), &st1);
		fstat(fileno(f), &st2);
		
		if (st1.st_ino == st2.st_ino && st1.st_dev == st2.st_dev)
		{
			fputs("Nope\n", stderr);
			exit(1);
		}
		
		oo = stdout;
		stdout = f;
		print(i);
		stdout = oo;
		
		if (fclose(f))
			goto fail;
		exit(0);
	}
	if (pid < 0)
	{
		perror(NULL);
		return 0;
	}
	while (wait(&st) != pid)
		;
	return !st;
fail:
	perror(path);
	exit(1);
	return 0; /* XXX silence a clang warning */
}

static void rmail(void)
{
	int i = msgcnt - 1;
	int nopr = 0;
	char buf[100];
	
	if (!msgcnt)
	{
		fputs("No mail\n", stderr);
		return;
	}
	
	for (;;)
	{
		if (nopr)
			nopr = 0;
		else
			print(i);
		
		fputs("? ", stdout);
		fflush(stdout);
		
		if (!fgets(buf, sizeof buf, stdin))
			break;
		
		switch (*buf)
		{
		case '-':
			if (i < msgcnt - 1)
				i++;
			break;
		case 'x':
			return;
		case 'q':
			goto fini;
		case 's':
			if (!save(i, buf + 1))
				break;
			msgdel[i] = 1;
			msgmod = 1;
			if (!i--)
				goto fini;
			break;
		case 'd':
			msgdel[i] = 1;
			msgmod = 1;
		case '\n':
		case '+':
			if (!i--)
				goto fini;
			break;
		case 'p':
			break;
		case 'h':
			puts(
				"+        next message\n"
				"-        previus message\n"
				"d        delete current message\n"
				"p        print current message\n"
				"q        quit\n"
				"s[name]  save to a file\n"
				"x        exit without updating mailbox"
			);
			nopr = 1;
			break;
		default:
			fputs("Invalid command (type h for help)\n", stderr);
			nopr = 1;
		}
	}
fini:
	if (msgmod)
		update();
}

int main(int argc, char **argv)
{
	struct passwd *pw;
	
	um = umask(077);
	
	pw = getpwuid(getuid());
	if (!pw)
	{
		fputs("No password file entry\n", stderr);
		return 1;
	}
	user = strdup(pw->pw_name);
	if (!user)
	{
		perror(NULL);
		return 1;
	}
	
	if (argc > 1 && !strcmp(argv[1], "-f"))
	{
		setgid(getgid());
		setuid(getuid());
		fflag = 1;
		
		strcpy(mailbox, pw->pw_dir);
		if (argc > 2)
		{
			strcat(mailbox, "/");
			strcat(mailbox, argv[2]);
		}
		else
			strcat(mailbox, "/mbox");
		
		load();
		rmail();
		return xit;
	}
	
	if (argc > 1)
	{
		send(argv + 1);
		return xit;
	}
	
	if (!*mailbox)
	{
		strcpy(mailbox, SPOOLDIR);
		strcat(mailbox, pw->pw_name);
	}
	
	load();
	rmail();
	
	return xit;
}
