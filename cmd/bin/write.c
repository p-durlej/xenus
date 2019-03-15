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
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <utmp.h>
#include <pwd.h>

static char *sname;
static char *dtty = NULL;
static char *stty;
static char *u;
static uid_t uid;
static int   dttyfd = -1;

static void findtty(void)
{
	struct stat st;
	struct utmp ut;
	int fd;
	int f = 0;
	int n = 0;
	
	fd = open("/etc/utmp", O_RDONLY);
	while (read(fd, &ut, sizeof ut) == sizeof ut)
	{
		if (dtty && strncmp(ut.ut_line, dtty, sizeof ut.ut_line))
			continue;
		if (strncmp(ut.ut_name, u, sizeof ut.ut_name))
			continue;
		if (stat(ut.ut_line, &st))
			continue;
		if (st.st_mode & 020)
		{
			f = 1;
			break;
		}
		n = 1;
	}
	close(fd);
	
	if (f)
	{
		dtty = strdup(ut.ut_line);
		return;
	}
	
	if (n)
		fprintf(stderr, "User %s has messages disabled\n", u);
	else if (dtty)
		fprintf(stderr, "User %s not logged in at %s\n", u, dtty);
	else
		fprintf(stderr, "User %s not logged in\n", u);
	exit(1);
}

static void sigint()
{
	write(dttyfd, "EOF\n", 4);
	_exit(0);
}

static void dowrite(void)
{
	void *ph;
	FILE *f;
	int c;
	
	f = fopen(dtty, "w");
	if (!f)
	{
		perror(dtty);
		exit(1);
	}
	dttyfd = fileno(f);
	setlinebuf(f);
	
	ph = signal(SIGINT, sigint);
	fprintf(f, "\nMessage from %s on %s\n", sname, stty);
	while (c = getchar(), c != EOF)
		fputc(c, f);
	signal(SIGINT, ph);
	
	fputs("EOF\n", f);
}

int main(int argc, char **argv)
{
	struct passwd *pw;
	
	if (argc < 2 || argc > 3)
	{
		fputs("write user [tty]\n", stderr);
		return 1;
	}
	
	if (argc > 2)
		dtty = argv[2];
	u = argv[1];
	
	pw = getpwnam(u);
	if (!pw)
	{
		fprintf(stderr, "User %s not found\n", u);
		return 1;
	}
	uid = pw->pw_uid;
	
	pw = getpwuid(getuid());
	if (!pw)
	{
		fprintf(stderr, "User not found\n");
		return 1;
	}
	sname = strdup(pw->pw_name);
	
	endpwent();
	
	stty = ttyname(0);
	if (!stty)
	{
		perror(NULL);
		return 1;
	}
	stty = strrchr(stty, '/') + 1;
	
	if (chdir("/dev"))
	{
		perror("/dev");
		return 1;
	}
	
	findtty();
	dowrite();
	return 0;
}
