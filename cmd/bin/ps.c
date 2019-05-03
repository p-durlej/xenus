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

#include <xenus/procdev.h>
#include <xenus/config.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

static struct procinfo procs[MAXPROCS];

struct dev
{
	struct dev *next;
	char *name;
	dev_t dev;
} *devs;

void ldevs(void)
{
	char path[NAME_MAX + 6] = "/dev/";
	struct dirent *de;
	struct stat st;
	struct dev *nd;
	DIR *d;
	
	d = opendir("/dev");
	if (d == NULL)
		return;
	
	while (de = readdir(d), de)
	{
		strcpy(path + 5, de->d_name);
		if (stat(path, &st))
			continue;
		
		if (!S_ISCHR(st.st_mode))
			continue;
		
		nd = sbrk(sizeof *nd);
		if (!nd)
			goto fail;
		
		nd->name = strdup(de->d_name);
		nd->next = devs;
		nd->dev  = st.st_rdev;
		
		devs = nd;
	}
fail:
	closedir(d);
}

void prdev(dev_t dev)
{
	struct dev *dp;
	char buf[22];
	
	for (dp = devs; dp; dp = dp->next)
		if (dp->dev == dev)
			break;
	if (dp)
	{
		printf("%-8s", dp->name);
		return;
	}
	
	sprintf(buf, "%i,%i", major(dev), minor(dev));
	printf("%-8s", buf);
}

static int pcmp(struct procinfo *p1, struct procinfo *p2)
{
	if (p1->tty != p2->tty)
		return p1->tty - p2->tty;
	return p1->pid - p2->pid;
}

int main(int argc, char **argv)
{
	struct procinfo *pi, *end;
	struct stat st;
	uid_t uid = getuid();
	int aflag = 0;
	int xflag = 0;
	int lflag = 0;
	char ttyn[8];
	char *p;
	int cnt;
	int fd;
	
	if (argc > 1)
		for (p = argv[1]; *p; p++)
			switch (*p)
			{
			case 'l':
				lflag = 1;
				break;
			case 'a':
				aflag = 1;
				break;
			case 'x':
				xflag = 1;
				break;
			case '-':
				break;
			default:
				fprintf(stderr, "ps: bad option '%c'\n", *p);
				return 1;
			}
	
	fd = open("/dev/proc", O_RDONLY);
	if (fd < 0)
	{
		perror("/dev/proc");
		return 1;
	}
	fstat(fd, &st);
	
	if (uid && !(st.st_mode & 4))
		aflag = 0;
	
	if (lflag)
		printf("TTY     SIZE PTABL PID  PPID PSID EUID RUID EGID RGID TS  COMPAT COMMAND\n");
	else
		printf("TTY     PID  COMMAND\n");
	ldevs();
	
	pi = procs;
	
	while (cnt = read(fd, pi, sizeof *pi), cnt)
	{
		if (cnt < 0)
		{
			perror("/dev/proc");
			return 1;
		}
		if (cnt != sizeof *pi)
		{
			fputs("/dev/proc: partial read\n", stderr);
			return 1;
		}
		if (!pi->pid)
			continue;
		
		if (!aflag && pi->ruid != uid)
			continue;
		
		if (pi->tty == -1 && !xflag)
			continue;
		
		pi++;
	}
	end = pi;
	
	qsort(procs, end - procs, sizeof *procs, (void *)pcmp);
	
	for (pi = procs; pi < end; pi++)
	{
		if (pi->tty != -1)
			prdev(pi->tty);
		else
			fputs("        ", stdout);
		
		if (lflag)
			printf("%4i %05x %-4i %-4i %-4i %-4i %-4i %-4i %-4i %-3i %-6s %s\n",
			       (int)pi->size >> 10,
			       (unsigned)pi->ptab >> 12,
			       (int)pi->pid,
			       (int)pi->ppid,
			       (int)pi->psid,
			       (int)pi->euid,
			       (int)pi->ruid,
			       (int)pi->egid,
			       (int)pi->rgid,
			       (int)pi->time_slice,
			       pi->compat,
			       pi->comm
			       );
		else
			printf("%-4i %s\n", (int)pi->pid, pi->comm);
	}
	return 0;
}
