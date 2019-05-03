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

#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

#define TAR_LT_REG	'0'
#define TAR_LT_HARD	'1'
#define TAR_LT_SYMBOLIC	'2'
#define TAR_LT_CHR	'3'
#define TAR_LT_BLK	'4'
#define TAR_LT_DIR	'5'
#define TAR_LT_FIFO	'6'

struct tar_hdr
{
	char path[100];
	char mode[8];
	char owner[8];
	char group[8];
	char size[12];
	char mtime[12];
	char cksum[8];
	char link_type;
	char link[100];
};

static char *tar_path = "/dev/rfd0";
static char **members;

static int pflag;
static int vflag;
static int fflag;
static int mode;

static int xit;

struct namenode
{
	struct namenode *next;
	ino_t		 ino;
	dev_t		 dev;
	char *		 path;
} *namenodes;

static void store1(int fd, char *path);

static int otoi(char *nptr)
{
	int n = 0;
	
	while (*nptr >= '0' && *nptr <= '7')
	{
		n <<= 3;
		n  += *nptr++ - '0';
	}
	return n;
}

static void add_namenode(char *path, dev_t dev, ino_t ino)
{
	struct namenode *nn;
	
	nn = malloc(sizeof *nn);
	if (!nn)
	{
		perror(NULL);
		exit(1);
	}
	
	nn->path = strdup(path);
	nn->ino = ino;
	nn->dev = dev;
	
	nn->next = namenodes;
	namenodes = nn;
}

static struct namenode *find_namenode(dev_t dev, ino_t ino)
{
	struct namenode *nn;
	
	for (nn = namenodes; nn; nn = nn->next)
		if (nn->dev == dev && nn->ino == ino)
			return nn;
	return NULL;
}

static char *spath(char *path)
{
	static char rpath[PATH_MAX + 1];
	
	char cpath[PATH_MAX];
	char *name;
	char *dp;
	
	while (*path == '/')
		path++;
	
	strcpy(cpath, path);
	*rpath = 0;
	dp = rpath;
	
	name = strtok(cpath, "/");
	
	while (name)
	{
		if (!strcmp(name, "."))
			goto next;
		
		if (!strcmp(name, ".."))
		{
			while (dp > rpath)
				if (*--dp == '/')
					break;
			goto next;
		}
		
		strcat(rpath, "/");
		strcat(rpath, name);
		
next:
		name = strtok(NULL, "/");
	}
	
	if (!rpath[1])
		return ".";
	
	return rpath + 1;
}

static int mused(char *path)
{
	int i;
	
	if (!*members)
		return 1;
	
	for (i = 0; members[i]; i++)
		if (!strcmp(members[i], path))
			return 1;
	return 0;
}

static void mkmstr(char *buf, mode_t mode, int lt)
{
	strcpy(buf, "?---------");
	
	switch (lt)
	{
	case '7':
	case '0':
	case 0:
		buf[0] = '-';
		break;
	case '1':
		buf[0] = 'h';
		break;
	case '2':
		buf[0] = 'l';
		break;
	case '3':
		buf[0] = 'c';
		break;
	case '4':
		buf[0] = 'b';
		break;
	case '5':
		buf[0] = 'd';
		break;
	case '6':
		buf[0] = 'p';
		break;
	default:
		;
	}
	
	if (mode & S_IRUSR)
		buf[1] = 'r';
	
	if (mode & S_IWUSR)
		buf[2] = 'w';
	
	if (mode & S_IXUSR)
		buf[3] = 'x';
	
	if (mode & S_ISUID)
		buf[3] = 's';
	
	if (mode & S_IRGRP)
		buf[4] = 'r';
	
	if (mode & S_IWGRP)
		buf[5] = 'w';
	
	if (mode & S_IXGRP)
		buf[6] = 'x';
	
	if (mode & S_ISGID)
		buf[6] = 's';
	
	if (mode & S_IROTH)
		buf[7] = 'r';
	
	if (mode & S_IWOTH)
		buf[8] = 'w';
	
	if (mode & S_IXOTH)
		buf[9] = 'x';
}

static void list1(int fd, struct tar_hdr *h, char *path, off_t size)
{
	time_t mtime = otoi(h->mtime);
	mode_t mode  = otoi(h->mode);
	char mtstr[32];
	char mstr[16];
	struct tm *tm;
	
	if (vflag > 1)
	{
		mkmstr(mstr, mode, h->link_type);
		
		tm = localtime(&mtime);
		printf("%s %3u/%-3u %8li ", mstr, otoi(h->owner), otoi(h->group), (long)size);
		
		printf("%04u-%02u-%02u %02u:%02u ",
			tm->tm_year + 1900, tm->tm_mon, tm->tm_mday,
			tm->tm_hour, tm->tm_min);
		
		if (h->link_type == '1')
			printf("%s linked to %s\n", h->path, h->link);
		else
			puts(path);
		return;
	}
	
	if (vflag)
		printf("%s\n", path);
}

static void shdr(int fd, void *hdr)
{
	ssize_t wcnt;
	
	wcnt = write(fd, hdr, 512);
	if (wcnt < 0)
	{
		perror(tar_path);
		exit(1);
	}
	if (wcnt != 512)
	{
		fprintf(stderr, "%s: Short write\n", tar_path);
		exit(1);
	}
}

static void sreg(int fd, struct tar_hdr *hd, char *path, struct stat *st)
{
	char buf[32768];
	ssize_t rwcnt, wcnt;
	ssize_t rcnt;
	int sfd;
	
	sfd = open(path, O_RDONLY);
	if (sfd < 0)
	{
		perror(path);
		xit = 1;
		return;
	}
	
	shdr(fd, hd);
	
	while (rcnt = read(sfd, buf, sizeof buf), rcnt > 0)
	{
		rwcnt = (rcnt + 511) & ~511;
		memset(buf + rcnt, 0, rwcnt - rcnt);
		
		wcnt = write(fd, buf, rwcnt);
		if (wcnt < 0)
		{
			perror(tar_path);
			exit(1);
		}
		if (wcnt != rwcnt)
		{
			fprintf(stderr, "%s: Short write\n", tar_path);
			exit(1);
		}
	}
	if (rcnt < 0)
	{
		perror(path);
		xit = 1;
	}
	
	close(sfd);
}

static void sdir(int fd, struct tar_hdr *hd, char *path)
{
	char path2[PATH_MAX];
	struct dirent *de;
	DIR *d;
	
	shdr(fd, hd);
	
	d = opendir(path);
	if (!d)
	{
		perror(path);
		xit = 1;
		return;
	}
	
	while (errno = 0, de = readdir(d), de)
	{
		if (!strcmp(de->d_name, "."))
			continue;
		
		if (!strcmp(de->d_name, ".."))
			continue;
		
		sprintf(path2, "%s/%s", path, de->d_name); // XXX
		store1(fd, path2);
	}
	if (errno)
	{
		perror(path);
		xit = 1;
	}
	
	closedir(d);
}

static void sspec(int fd, struct tar_hdr *hd, char *path, struct stat *st)
{
	shdr(fd, hd);
}

static void init_head(struct tar_hdr *hd, struct stat *st, char *path)
{
	if (strlen(path) >= sizeof hd->path)
	{
		errno = ENAMETOOLONG;
		perror(path);
		xit = 1;
		return;
	}
	
	strcpy(hd->path, path);
	
	sprintf(hd->mode,  "%07o",  (int)st->st_mode);
	sprintf(hd->owner, "%07o",  (int)st->st_uid);
	sprintf(hd->group, "%07o",  (int)st->st_gid);
	sprintf(hd->size,  "%011o", (int)st->st_size);
	sprintf(hd->mtime, "%011o", (int)st->st_mtime);
	
	switch (st->st_mode & S_IFMT)
	{
	case S_IFREG:
		hd->link_type = TAR_LT_REG;
		break;
	case S_IFDIR:
		hd->link_type = TAR_LT_DIR;
		break;
	default:
		hd->link_type = '0';
	}
}

static void cksum(struct tar_hdr *hd)
{
	unsigned char *sp;
	unsigned cksum;
	int i;
	
	memset(hd->cksum, ' ', sizeof hd->cksum);
	
	sp = (unsigned char *)hd;
	for (cksum = 0, i = 0; i < sizeof *hd; i++)
		cksum += sp[i];
	
	sprintf(hd->cksum, "%06o ", cksum);
}

static void store1(int fd, char *path)
{
	union { struct tar_hdr h; char buf[512]; } u;
	struct namenode *nn;
	struct stat st;
	int link = 0;
	
	if (stat(path, &st))
	{
		perror(path);
		xit = 1;
		return;
	}
	
	if (!S_ISREG(st.st_mode))
		st.st_size = 0;
	
	memset(&u, 0, sizeof u);
	init_head(&u.h, &st, path);
	
	if (st.st_nlink > 1)
	{
		nn = find_namenode(st.st_dev, st.st_ino);
		if (nn)
		{
			strcpy(u.h.size, "00000000000");
			strcpy(u.h.link, nn->path);
			u.h.link_type = TAR_LT_HARD;
			link = 1;
		}
		else
			add_namenode(path, st.st_dev, st.st_ino);
	}
	
	cksum(&u.h);
	
	if (!link)
		switch (st.st_mode & S_IFMT)
		{
		case S_IFREG:
			sreg(fd, &u.h, path, &st);
			break;
		case S_IFDIR:
			sdir(fd, &u.h, path);
			break;
		default:
			sspec(fd, &u.h, path, &st);
			break;
		}
	
	list1(fd, &u.h, path, st.st_size);
}

static void store(int fd)
{
	char endblk[1024];
	ssize_t wcnt;
	int i;
	
	if (!*members)
		fputs("No members\n", stderr);
	
	for (i = 0; members[i]; i++)
		store1(fd, members[i]);
	
	memset(endblk, 0, sizeof endblk);
	
	wcnt = write(fd, endblk, sizeof endblk);
	if (wcnt < 0)
	{
		perror(tar_path);
		exit(1);
	}
	if (wcnt != sizeof endblk)
	{
		fprintf(stderr, "%s: Short write\n", tar_path);
		exit(1);
	}
}

static void create(void)
{
	int fd;
	
	fd = open(tar_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (fd < 0)
	{
		perror(tar_path);
		exit(1);
	}
	
	store(fd);
	close(fd);
}

static void append(void)
{
	union { struct tar_hdr h; char buf[512]; } u;
	struct stat st;
	ssize_t cnt;
	off_t size;
	int fd;
	
	fd = open(tar_path, O_RDWR | O_CREAT, 0666);
	if (fd < 0)
		goto fail;
	
	if (fstat(fd, &st))
		goto fail;
	
	if (!S_ISBLK(st.st_mode) && !S_ISREG(st.st_mode))
	{
		fprintf(stderr, "%s: Not a reg/blk file\n", tar_path);
		exit(1);
	}
	
	while (cnt = read(fd, &u.buf, sizeof u.buf), cnt == sizeof u.buf)
	{
		size = otoi(u.h.size);
		
		if (!*u.h.path)
		{
			lseek(fd, -512, SEEK_CUR);
			break;
		}
		
		size +=  511;
		size &= ~511;
		
		lseek(fd, size, SEEK_CUR);
	}
	if (cnt < 0)
	{
		perror(tar_path);
		exit(1);
	}
	
	store(fd);
	close(fd);
	return;
fail:
	perror(tar_path);
	exit(1);
}

typedef void rtarf(int fd, struct tar_hdr *h, char *path, off_t size);

static void rtar(rtarf *func)
{
	union { struct tar_hdr h; char buf[512]; } u;
	off_t size;
	off_t off = 0;
	ssize_t cnt;
	int fd;
	int i;
	
	fd = open(tar_path, O_RDONLY);
	if (fd < 0)
	{
		perror(tar_path);
		exit(1);
	}
	
	while (cnt = read(fd, &u.buf, sizeof u.buf), cnt == sizeof u.buf)
	{
		if (!*u.h.path)
			break;
		size = otoi(u.h.size);
		
		if (mused(u.h.path))
			func(fd, &u.h, u.h.path, size);
		
		off += ((size + 511) & ~511) + 512;
		lseek(fd, off, SEEK_SET);
	}
	if (cnt < 0)
	{
		perror(tar_path);
		exit(1);
	}
	
	close(fd);
}

static void preserve(struct tar_hdr *h, int fd, char *path)
{
	int fdo = 0;
	
	if (!pflag)
		return;
	
	if (fd < 0)
	{
		fd = open(path, O_RDONLY); // XXX
		if (fd < 0)
		{
			perror(path);
			return;
		}
		fdo = 1;
	}
	
	if (fchown(fd, otoi(h->owner), otoi(h->group)))
	{
		perror(path);
		xit = 1;
		goto clean;
	}
	
	if (fchmod(fd, otoi(h->mode)))
	{
		perror(path);
		xit = 1;
	}
clean:
	if (fdo)
		close(fd);
}

static void xdir(struct tar_hdr *h, char *path)
{
	if (mkdir(path, otoi(h->mode)) && errno != EEXIST)
	{
		perror(path);
		xit = 1;
		return;
	}
	
	preserve(h, -1, path);
}

static void xreg(int fd, struct tar_hdr *h, char *path, off_t size)
{
	static char buf[32768];
	
	ssize_t isz, osz, rsz;
	int ofd;
	
	ofd = open(path, O_WRONLY | O_CREAT | O_TRUNC, otoi(h->mode) & 04777);
	if (ofd < 0)
	{
		perror(path);
		xit = 1;
		return;
	}
	
	while (size)
	{
		isz = size < sizeof buf ? size : sizeof buf;
		rsz = (isz + 511) & ~511;
		rsz = read(fd, buf, rsz);
		if (rsz < 0)
		{
			perror(tar_path);
			exit(1);
		}
		
		osz = write(ofd, buf, isz); // XXX
		if (osz < 0)
		{
			perror(path);
			xit = 1;
			break;
		}
		
		size -= isz;
	}
	
	preserve(h, ofd, path);
	close(ofd);
}

static void xlink(struct tar_hdr *h, char *path)
{
	char npath[PATH_MAX];
	char *opath;
	
	strcpy(npath, path);
	opath = spath(h->link);
	
	switch (h->link_type)
	{
	case TAR_LT_HARD:
		if (link(opath, npath))
		{
			fprintf(stderr, "%s -> %s: %m\n", npath, opath);
			xit = 1;
		}
		break;
	default:
		fprintf(stderr, "%s: Unknown link type", npath);
		xit = 1;
	}
}

static void xspec(struct tar_hdr *h, char *path)
{
	fprintf(stderr, "%s: Special file\n", path); // XXX
	xit = 1;
}

static void xfifo(struct tar_hdr *h, char *path)
{
	if (mkfifo(path, otoi(h->mode) & 0777))
	{
		perror(path);
		xit = 1;
	}
}

static void extract1(int fd, struct tar_hdr *h, char *path, off_t size)
{
	char *spn;
	
	spn = spath(path);
	list1(fd, h, path, size);
	
	switch (h->link_type)
	{
	case TAR_LT_REG:
	case 0:
		xreg(fd, h, spn, size);
		break;
	case TAR_LT_CHR:
	case TAR_LT_BLK:
		xspec(h, spn);
		break;
	case TAR_LT_FIFO:
		xfifo(h, spn);
		break;
	case TAR_LT_DIR:
		xdir(h, spn);
		break;
	case TAR_LT_HARD:
		xlink(h, spn);
		break;
	default:
		;
	}
}

static void usage(void)
{
	fputs("tar {crtux}[01pvv][f archive] [file...]\n", stderr);
	exit(1);
}

static int parse_opts(int argc, char **argv)
{
	int cnt = 0;
	char *p;
	
	if (argc < 1)
		usage();
	
	for (p = argv[0]; *p; p++)
		switch (*p)
		{
		case '0':
			tar_path = "/dev/rfd0"; /* The default anyway */
			break;
		case '1':
			tar_path = "/dev/rfd1";
			break;
		case 'c':
		case 'r':
		case 't':
		case 'x':
			mode = *p;
			cnt++;
			break;
		case 'u':
			mode = 'r';
			cnt++;
			break;
		case 'f':
			fflag = 1;
			break;
		case 'p':
			pflag = 1;
			break;
		case 'v':
			vflag++;
			break;
		default:
			usage();
		}
	
	if (mode == 't')
		vflag++;
	
	if (cnt != 1)
		usage();
	
	if (fflag)
	{
		if (argc < 2)
			usage();
		
		tar_path = argv[1];
		return 2;
	}
	
	return 1;
}

int main(int argc, char **argv)
{
	members = argv + parse_opts(argc - 1, argv + 1) + 1;
	
	switch (mode)
	{
	case 'c':
		create();
		break;
	case 'r':
		append();
		break;
	case 't':
		rtar(list1);
		break;
	case 'x':
		rtar(extract1);
		break;
	default:
		abort();
	}
	return xit;
}
