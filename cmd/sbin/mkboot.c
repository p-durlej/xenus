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
#include <sys/ioctl.h>
#include <sys/dioc.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#define BOOTCONF_OFF	0x1f0
#define PART_OFF	0x1d0
#define KERN_OFF	1024L

struct bootconf
{
	unsigned long	kern_offset;
	unsigned short	kern_seg;
	unsigned short	kern_size;
};

struct pcpart
{
	unsigned char boot;
	unsigned char schs[3];
	unsigned char id;
	unsigned char echs[3];
	unsigned start, size;
};

static struct bootconf *bootconf;
static char	bootsect[512];
static unsigned	part[7];
static char *	device;
static char *	image = "/usr/mdec/xenus";
static char *	boot = "/usr/mdec/boot";
static int	rootro;
static int	ndisks = 4;
static int	dsleep;
static int	cnmode = 3;
static int	offset;
static int	diskfd = -1;

static void loadpart(void)
{
	char buf[512];
	int cnt;
	
	lseek(diskfd, 0, SEEK_SET);
	
	cnt = read(diskfd, buf, sizeof buf);
	if (cnt < 0)
	{
		perror(device);
		exit(1);
	}
	if (cnt != 512)
	{
		fprintf(stderr, "%s: Short read\n", device);
		exit(1);
	}
	
	memcpy(part, buf + PART_OFF, sizeof part);
}

static void loadboot(void)
{
	int fd = open(boot, O_RDONLY);
	int cnt;
	
	if (fd < 0)
	{
		perror(boot);
		exit(1);
	}
	
	cnt = read(fd, bootsect, sizeof(bootsect));
	if (cnt < 0)
	{
		perror(boot);
		exit(1);
	}
	
	if (cnt != 512)
	{
		fprintf(stderr, "bad %s\n", boot);
		exit(1);
	}
	close(fd);
	bootconf = (struct bootconf *)(bootsect + BOOTCONF_OFF);
}

static void mkboot(void)
{
	struct stat st;
	dev_t rootdev;
	char buf[512];
	int cnt;
	int kfd;
	int z = 1;
	
	memcpy(bootsect + PART_OFF, part, sizeof part);
	bootconf->kern_offset = offset + 1;
	fstat(diskfd, &st);
	
	rootdev = st.st_rdev;
	
	if (lseek(diskfd, 0, SEEK_SET) < 0)
		goto dkerr;
	
	if (write(diskfd, bootsect, sizeof bootsect) != sizeof bootsect)
		goto dkerr;
	
	if (lseek(diskfd, KERN_OFF - sizeof bootsect, SEEK_CUR) < 0)
		goto dkerr;
	
	kfd = open(image, O_RDONLY);
	if (kfd < 0)
		goto dkerr;
	
	while (cnt = read(kfd, buf, sizeof(buf)), cnt)
	{
		if (cnt < 0)
			goto dkerr;
		
		if (z)
		{
			printf("root %i,%i\n", major(rootdev), minor(rootdev));
			if (rootro)
				printf("rootro %i\n", rootro);
			if (ndisks)
				printf("ndisks %i\n", ndisks);
			if (dsleep)
				printf("dsleep %i\n", dsleep);
			if (cnmode != 3)
				printf("cnmode %i\n", cnmode);
			memcpy(buf + 16, &rootdev, 4);
			memcpy(buf + 20, &rootro, 4);
			memcpy(buf + 24, &ndisks, 4);
			memcpy(buf + 28, &dsleep, 4);
			memcpy(buf + 40, &cnmode, 4);
			z = 0;
		}
		
		if (write(diskfd, buf, cnt) != cnt)
			goto dkerr;
	}
	
	close(kfd);
	close(diskfd); // XXX
	return;
dkerr:
	perror(device);
	exit(errno);
}

static void goffset(void)
{
	struct pcpart *p, part[4];
	
	lseek(diskfd, 0x1be, SEEK_SET);
	errno = EINVAL;
	if (read(diskfd, part, sizeof part) != sizeof part)
	{
		perror(device);
		exit(1);
	}
	
	for (p = part; p < part + 4; p++)
		if (p->id == 0xc0)
		{
			offset = p->start;
			printf("offset %i\n", offset);
			break;
		}
}

static int procopt(char *s)
{
	s++;
	
	while (*s)
		switch (*s++)
		{
		case 'r':
			rootro = 1;
			break;
		case 's':
			dsleep = 1;
			break;
		case 'm':
			cnmode = 7;
			break;
		case '-':
			break;
		default:
			if (s[-1] >= '0' && s[-1] <= '4')
			{
				ndisks = s[-1] - '0';
				break;
			}
			fprintf(stderr, "mkboot: bad option '%c'\n", s[-1]);
			exit(1);
		}
	
	return 1;
}

int main(int argc,char **argv)
{
	struct dinfo di;
	int i;
	
	for (i = 1; i < argc; i++)
	{
		if (*argv[i] != '-')
			break;
		if (!procopt(argv[i]))
			break;
	}
	argv += i;
	argc -= i;
	
	if (argc < 1 || argc > 2)
	{
		fputs("mkboot [-rsm01234] dev\n", stderr);
		return 255;
	}
	
	device = argv[0];
	diskfd = open(device, O_RDWR);
	if (diskfd < 0)
		goto dkerr;
	
	if (ioctl(diskfd, DIOCGINFO, &di))
		goto dkerr;
	if (di.offset)
	{
		printf("offset %i\n", di.offset);
		offset = di.offset;
	}
	
	loadpart();
	loadboot();
	
	mkboot();
	return 0;
dkerr:
	perror(device);
	return 1;
}
