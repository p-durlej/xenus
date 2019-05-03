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
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>

struct dent
{
	char *	name;
	ino_t	ino;
	mode_t	mode;
	nlink_t	nlink;
	uid_t	ugid;
	off_t	size;
	time_t	time;
	dev_t	rdev;
};

char *ugname;
uid_t ugid;

int longfmt  = 0;
int dotfiles = 0;
int dir	     = 0;
int ino	     = 0;
int multi    = 0;
int gflag    = 0;
int cflag;
int uflag;

time_t soy;

int procopt(char *s)
{
	s++;
	
	while (*s)
		switch (*s++)
		{
		case 'l':
			longfmt = 1;
			break;
		case 'a':
			dotfiles = 1;
			break;
		case 'c':
			cflag = 1;
			break;
		case 'd':
			dotfiles = 1;
			dir = 1;
			break;
		case 'i':
			ino = 1;
			break;
		case 'g':
			gflag = 1;
			break;
		case 'u':
			uflag = 1;
			break;
		case '-':
			break;
		default:
			fprintf(stderr, "ls: bad option '%c'\n", s[-1]);
			exit(1);
		}
	
	return 1;
}

void getugname(uid_t id)
{
	static char buf[16];
	
	struct passwd *owner;
	struct group *group;
	
	if (gflag)
	{
		if (ugid == id && ugname)
			return;
		
		group = getgrgid(id);
		if (group)
		{
			ugname = group->gr_name;
			ugid   = id;
		}
		else
		{
			sprintf(buf, "%i", id);
			ugname = buf;
			ugid   = id;
		}
		
		return;
	}
	
	if (ugid == id && ugname)
		return;
	
	owner = getpwuid(id);
	if (owner)
	{
		ugname = owner->pw_name;
		ugid   = id;
	}
	else
	{
		sprintf(buf, "%i", id);
		ugname = buf;
		ugid   = id;
	}
}

void ptime(time_t t)
{
	static char *mon[] =
	{
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
	
	struct tm *tm = localtime(&t);
	
	if (t < soy)
	{
		printf("%s %2i  %i ", mon[tm->tm_mon], tm->tm_mday,
			tm->tm_year + 1900);
		return;
	}
	
	printf("%s %2i %02i:%02i ", mon[tm->tm_mon], tm->tm_mday,
			tm->tm_hour, tm->tm_min);
}

void pdirent(struct dent *de)
{
	if (*de->name == '.' && !dotfiles)
		return;
	
	if (longfmt)
	{
		char perm[11];
		char *nl;
		
		strcpy(perm, "----------");
		
		switch (S_IFMT & de->mode)
		{
		case S_IFREG:
			perm[0] = '-';
			break;
		case S_IFDIR:
			perm[0] = 'd';
			break;
		case S_IFCHR:
			perm[0] = 'c';
			break;
		case S_IFBLK:
			perm[0] = 'b';
			break;
		case S_IFIFO:
			perm[0] = 'p';
			break;
		default:
			perm[0] = '?';
		}
		
		if (de->mode & S_IRUSR)
			perm[1] = 'r';
		if (de->mode & S_IWUSR)
			perm[2] = 'w';
		if (de->mode & S_IXUSR)
			perm[3] = 'x';
			
		if (de->mode & S_IRGRP)
			perm[4] = 'r';
		if (de->mode & S_IWGRP)
			perm[5] = 'w';
		if (de->mode & S_IXGRP)
			perm[6] = 'x';
			
		if (de->mode & S_IROTH)
			perm[7] = 'r';
		if (de->mode & S_IWOTH)
			perm[8] = 'w';
		if (de->mode & S_IXOTH)
			perm[9] = 'x';
		
		if (de->mode & S_ISUID)
			perm[3] = (de->mode & 0100) ? 's' : 'S';
		if (de->mode & S_ISGID)
			perm[6] = (de->mode & 0010) ? 's' : 'S';
		
		if (de->mode & S_ISVTX)
			perm[9] = (de->mode & 0001) ? 't' : 'T';
		
		if (ino)
			printf("%6i ", (int)de->ino);
		
		getugname(de->ugid);
		
		printf("%s %2i ", perm, de->nlink);
		printf("%-8s ", ugname);
		
		switch (S_IFMT & de->mode)
		{
		case S_IFCHR:
		case S_IFBLK:
			printf("%3i,%3i ", major(de->rdev), minor(de->rdev));
			break;
		default:
			printf("%7i ", (int)de->size);
		}
		
		ptime(de->time);
		puts(de->name);
	}
	else
	{
		if (ino)
			printf("%8i ", (int)de->ino);
		printf("%s\n", de->name);
	}
}

static void fillde(struct dent *de, struct stat *st)
{
	de->ino	  = st->st_ino;
	de->mode  = st->st_mode;
	de->nlink = st->st_nlink;
	de->ugid  = gflag ? st->st_gid : st->st_uid;
	de->size  = st->st_size;
	de->time  = st->st_mtime;
	de->rdev  = st->st_rdev;
	
	if (cflag)
		de->time = st->st_ctime;
	if (uflag)
		de->time = st->st_atime;
}

int dcmp(void *p1, void *p2)
{
	struct dent *d1 = p1;
	struct dent *d2 = p2;
	
	return strcmp(d1->name, d2->name);
}

void list_dir(char *path)
{
	static struct dent dents[256];
	
	char filename[PATH_MAX];
	struct dirent *de;
	struct stat st;
	DIR *dir;
	blkcnt_t total = 0;
	int cnt = 0;
	int i;
	
	if (multi)
		printf("%s:\n", path);
	
	dir = opendir(path);
	if (!dir)
	{
		perror(path);
		exit(255);
	}
	
	while (de = readdir(dir), de)
	{
		if (*de->d_name == '.' && !dotfiles)
			continue;
		
		if (cnt > sizeof dents / sizeof *dents)
		{
			fprintf(stderr, "%s: too many entries\n", path);
			exit(1);
		}
		
		dents[cnt].name	= strdup(de->d_name);
		dents[cnt].ino	= de->d_ino;
		if (!dents[cnt].name)
		{
			perror(NULL);
			exit(1);
		}
		
		if (longfmt)
		{
			snprintf(filename, PATH_MAX, "%s/%s", path, dents[cnt].name);
			if (stat(filename, &st))
			{
				perror(filename);
				exit(1);
			}
			fillde(&dents[cnt], &st);
			total += st.st_blocks;
		}
		
		cnt++;
	}
	
	if (longfmt)
		printf("total %u\n", total);
	
	qsort(dents, cnt, sizeof *dents, dcmp);
	
	for (i = 0; i < cnt; i++)
	{
		pdirent(&dents[i]);
		free(dents[i].name);
	}
}

void list(char *path)
{
	struct dent de;
	struct stat st;
	
	if (stat(path, &st))
	{
		perror(path);
		exit(255);
	}
	
	if (S_ISDIR(st.st_mode) && !dir)
		list_dir(path);
	else
	{
		de.name  = path;
		fillde(&de, &st);
		pdirent(&de);
	}
}

int main(int argc, char **argv)
{
	char *p;
	int i;
	
	p = strrchr(argv[0], '/');
	if (p)
		p++;
	else
		p = argv[0];
	
	if (!strcmp(p, "l"))
		longfmt = 1;
	
	if (argc == 2 && !strcmp(argv[1], "-h"))
	{
		puts("ls [-acdgilu] [file1...]");
		return 0;
	}
	
	for (i = 1; i < argc; i++)
	{
		if (*argv[i] != '-')
			break;
		if (!procopt(argv[i]))
			break;
	}
	
	if (argc - i > 1)
		multi = 1;
	
	if (i == argc)
	{
		list(".");
		return 0;
	}
	
	if (longfmt)
	{
		struct tm tm;
		time_t t;
		
		time(&t);
		tm = *localtime(&t);
		tm.tm_mday = 1;
		tm.tm_mon  = 0;
		
		soy = mktime(&tm);
	}
	
	for (; i < argc; i++)
		list(argv[i]);
	return 0;
}
