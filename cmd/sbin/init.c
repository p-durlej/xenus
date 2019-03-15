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

#include <xenus/config.h>
#include <xenus/io.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <utmp.h>
#include <time.h>

#define IOBASE		0x70
#define NTASKS		32

int _ctty(char *path);
int _iopl(void);

struct init_task
{
	pid_t pid;
	char *tty;
	char  type;
};

struct init_task task[NTASKS];

unsigned bcdint(unsigned v)
{
	return (v >> 4) * 10 + (v & 0x0f);
}

void hwclock()
{
	char buf[16] = "";
	struct tm tm;
	time_t t;
	FILE *f;
	
	_iopl();
	do
	{
		outb(IOBASE, 0x00);
		tm.tm_sec = bcdint(inb(IOBASE + 1));
		
		outb(IOBASE, 0x02);
		tm.tm_min = bcdint(inb(IOBASE + 1));
		
		outb(IOBASE, 0x04);
		tm.tm_hour = bcdint(inb(IOBASE + 1));
		
		outb(IOBASE, 0x07);
		tm.tm_mday = bcdint(inb(IOBASE + 1));
		
		outb(IOBASE, 0x08);
		tm.tm_mon = bcdint(inb(IOBASE + 1)) - 1;
		
		outb(IOBASE, 0x09);
		tm.tm_year = bcdint(inb(IOBASE + 1));
		
		outb(IOBASE,0x00);
	}
	while (tm.tm_sec != bcdint(inb(IOBASE + 1)));
	
	f = fopen("/etc/year", "r");
	if (f)
	{
		if (fgets(buf, sizeof buf, f))
			tm.tm_year = atoi(buf) - 2000;
		fclose(f);
	}
	
	if (tm.tm_year < 18 || tm.tm_year >= 38)
	{
		fputs("Invalid date\n", stderr);
		tm.tm_year = 18;
	}
	
	tm.tm_year += 100;
	t = timegm(&tm);
	stime(&t);
}

void single()
{
	pid_t pid;
	
	pid = fork();
	if (!pid)
	{
		if (_ctty("/dev/console"))
			fputs("can't ctty\n", stderr);
		
		execl("/bin/sh", "/bin/sh", NULL);
		perror("/bin/sh");
	}
	if (pid < 0)
	{
		perror("fork");
		return;
	}
	
	while (wait(NULL) != pid);
}

void readtab()
{
	FILE *f;
	int i;
	
	f = fopen("/etc/ttys", "r");
	if (!f)
		goto fail;
	
	for (i = 0; i < NTASKS; i++)
	{
		char buf[256];
		char *p;
		
		clearerr(f);
		fgets(buf, sizeof(buf), f);
		if (ferror(f))
			goto fail;
		if (feof(f))
		{
			fclose(f);
			return;
		}
		p = strchr(buf, '\n');
		if (p)
			*p = 0;
		
		if (strlen(buf) < 3)
			goto fail;
		
		task[i].tty  = strdup(buf + 2);
		task[i].type = buf[1];
		
		if (buf[0] == '0')
			task[i].type = 0;
		
		if (!task[i].tty)
			goto fail;
	}
	return;
fail:
	perror("init: /etc/ttys");
	single();
}

void respawn()
{
	struct init_task *t;
	int i;
	
	for (i = 0; i < NTASKS; i++)
	{
		t = &task[i];
		
		if (!t->pid && t->tty)
		{
			char ty[2] = "X";
			pid_t pid;
			
			pid = fork();
			if (pid < 0)
			{
				perror("init: fork");
				continue;
			}
			
			if (!pid)
			{
				if (setsid())
				{
					perror("init: setsid");
					_exit(255);
				}
				
				ty[0] = t->type;
				execl("/bin/getty", "getty", t->tty, ty, (void *)NULL);
				_exit(255);
			}
			t->pid = pid;
		}
	}
}

void dead(pid_t pid, int status)
{
	struct utmp ut;
	char *n;
	int fd;
	int i;
	
	for (i = 0; i < NTASKS; i++)
		if (task[i].pid == pid)
		{
			task[i].pid = 0;
			break;
		}
	if (i >= NTASKS)
		return;
	
	strncpy(ut.ut_line, task[i].tty, sizeof ut.ut_line);
	
	fd = open("/etc/utmp", O_WRONLY);
	if (fd < 0)
	{
		if (errno != EROFS && errno != ENOENT)
			perror("init: /etc/utmp");
		return;
	}
	lseek(fd, i * sizeof ut, SEEK_SET);
	write(fd, &ut, sizeof ut);
	close(fd);
}

void sigterm(int nr)
{
	signal(nr, SIG_IGN);
	for (;;)
		pause();
}

void mkutmp(void)
{
	struct utmp ut;
	int fd;
	int i;
	
	fd = open("/etc/utmp", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0)
	{
		if (errno != EROFS)
			perror("/etc/utmp");
		return;
	}
	
	for (i = 0; i < NTASKS; i++)
		if (task[i].tty)
		{
			memset(&ut, 0, sizeof ut);
			strncpy(ut.ut_line, task[i].tty, sizeof ut.ut_line);
			if (write(fd, &ut, sizeof ut) < 0)
			{
				perror("/etc/utmp");
				break;
			}
		}
	
	close(fd);
}

void runrc()
{
	pid_t pid;
	
	if (access("/etc/rc", 0))
		return;
	
	pid = fork();
	if (!pid)
	{
		signal(SIGQUIT, SIG_DFL);
		signal(SIGINT, SIG_DFL);
		
		execl("/bin/sh", "/bin/sh", "/etc/rc", NULL);
		perror("/bin/sh");
		_exit(255);
	}
	if (pid < 0)
	{
		perror("/bin/sh: fork");
		return;
	}
	while (wait(NULL) != pid);
}

int main(int argc, char **argv, char **envp)
{
	int status;
	pid_t pid;
	
	if (getpid() != 1)
	{
		fprintf(stderr, "Not permitted\n");
		return 1;
	}
	
	signal(SIGTERM, sigterm);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	
	hwclock();
	// single();
	runrc();
	
	_ctty(NULL);
	readtab();
	mkutmp();
	
	for (;;)
	{
		respawn();
		pid = wait(&status);
		if (pid > 1)
			dead(pid, status);
	}
}
