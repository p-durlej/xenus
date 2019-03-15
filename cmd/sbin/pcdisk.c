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

#include <sys/ioctl.h>
#include <sys/dioc.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

#define ID 192

static char mbr[512];

static struct pcpart
{
	unsigned char boot;
	unsigned char schs[3];
	unsigned char id;
	unsigned char echs[3];
	unsigned start, size;
} *part = (void *)(mbr + 0x1be);

static char *devname = "/dev/rhd7";
static int c = 1024, h = 16, s = 63;
static int devfd;
static int cflag;
static int dflag;
static int aflag;
static int bflag;
static int gflag;
static int pflag;
static int nflag;
static int bigX;
static int actn = -1;
static int size = 80000;
static int modf, rtbl;
static int xit;

static void opendev(void)
{
	struct dinfo di;
	
	devfd = open(devname, O_RDWR);
	if (devfd < 0)
	{
		perror(devname);
		exit(1);
	}
	
	errno = EINVAL;
	if (read(devfd, mbr, sizeof mbr) != sizeof mbr)
	{
		perror(devname);
		exit(1);
	}
	
	if (ioctl(devfd, DIOCGINFO, &di))
	{
		perror(devname);
		exit(1);
	}
	
	c = di.c;
	h = di.h;
	s = di.s;
}

static void gchs1(unsigned char *raw, int *out)
{
	out[0] = raw[2] | ((raw[1] & 0xc0) << 2);
	out[1] = raw[0];
	out[2] = raw[1] & 63;
}

static void gchs(struct pcpart *p, int *schs, int *echs)
{
	gchs1(p->schs, schs);
	gchs1(p->echs, echs);
}

static void schs1(unsigned char *raw, int *in)
{
	raw[0] = in[1];
	raw[1] = (in[2] & 63) | ((in[0] >> 2) & 0xc0);
	raw[2] = in[0];
}

static void schs(struct pcpart *p, int *schs, int *echs)
{
	schs1(p->schs, schs);
	schs1(p->echs, echs);
}

static void prpart(struct pcpart *p, int i)
{
	int schs[3], echs[3];
	
	if (p->id)
	{
		gchs(p, schs, echs);
		printf("%i %c  %02x  (%4i,%2i,%2i) - (%4i,%2i,%2i)  %7i : %7i blocks\n",
			i,
			p->boot ? 'x' : '-',
			p->id,
			schs[0], schs[1], schs[2],
			echs[0], echs[1], echs[2],
			p->start, p->size);
	}
}

static void prtab(void)
{
	int i;
	
	for (i = 0; i < 4; i++)
		prpart(&part[i], i);
}

static void sactiv(void)
{
	int i;
	
	for (i = 0; i < 4; i++)
		if (part[i].id == ID)
		{
			part[i].boot = 128;
			break;
		}
	if (i >= 4)
	{
		fputs("Not found\n", stderr);
		xit = 1;
		return;
	}
	
	for (i = 0; i < 4; i++)
		if (part[i].id != ID)
			part[i].boot = 0;
	modf = 1;
}

static void sactn(void)
{
	int i;
	
	if (actn >= 4 || !part[actn].id)
	{
		fputs("No such partition\n", stderr);
		xit = 1;
		return;
	}
	
	for (i = 0; i < 4; i++)
		part[i].boot = 0;
	part[actn].boot = 128;
	modf = 1;
}

static void cpart(void)
{
	int pschs[3] = { 1, 0, 1 }, pechs[3] = { 1, h - 1, s };
	struct pcpart *p;
	int nc;
	int i;
	
	for (i = 0; i < 4; i++)
	{
		int pechs1[3];
		
		if (part[i].id == ID)
		{
			fputs("Already exists\n", stderr);
			xit = 1;
			return;
		}
		if (!part[i].id)
			continue;
		
		gchs1(part[i].echs, pechs1);
		if (pechs1[0] >= pschs[0])
			pschs[0] = pechs1[0] + 1;
	}
	
	for (i = 0; i < 4; i++)
		if (!part[i].id)
		{
			p = &part[i];
			break;
		}
	if (i >= 4)
	{
		fputs("Table full\n", stderr);
		xit = 1;
		return;
	}
	
	nc = (size + s - 1) / s;
	nc = (nc   + h - 1) / h;
	
	pechs[0] = pschs[0] + nc - 1;
	
	memset(p, 0, sizeof *p);
	schs(p, pschs, pechs);
	p->start = pschs[0] * h * s;
	p->size	 = nc * h * s;
	p->id	 = ID;
	rtbl = modf = 1;
}

static void dpart(void)
{
	int i;
	
	for (i = 0; i < 4; i++)
		if (part[i].id == ID)
		{
			memset(&part[i], 0, sizeof part[i]);
			rtbl = modf = 1;
			break;
		}
	if (i >= 4)
	{
		fputs("Not found\n", stderr);
		xit = 1;
		return;
	}
}

static void setipl(void)
{
	int fd = -1;
	
	fd = open("/usr/mdec/pcmbr", O_RDONLY);
	if (fd < 0)
		goto fail;
	if (read(fd, mbr, 446) < 0)
		goto fail;
	close(fd);
	mbr[510] = 0x55;
	mbr[511] = 0xaa;
	modf = 1;
	return;
fail:
	perror("/usr/mdec/pcmbr");
	xit = 1;
	if (fd >= 0)
		close(fd);
}

static void save(void)
{
	if (!modf)
		return;
	
	lseek(devfd, 0, SEEK_SET);
	if (write(devfd, mbr, sizeof mbr) != sizeof mbr)
	{
		perror(devname);
		xit = 1;
	}
	if (rtbl)
		ioctl(devfd, DIOCRTBL, 0);
}

static void clear(void)
{
	memset(part, 0, 4 * sizeof *part);
	modf = 1;
}

int main(int argc, char **argv)
{
	int flag = 0;
	char *p;
	
	if (*argv[1] == '-')
	{
		for (p = argv[1] + 1; *p; p++)
			switch (*p)
			{
			case 'a':
				flag = aflag = 1;
				break;
			case 'b':
				flag = bflag = 1;
				break;
			case 'c':
				flag = cflag = 1;
				break;
			case 'd':
				flag = dflag = 1;
				break;
			case 'g':
				flag = gflag = 1;
				break;
			case 'l':
			case 'p':
				flag = pflag = 1;
				break;
			case 'n':
				nflag = 1;
				break;
			case 'X':
				flag = bigX = 1;
				break;
			case 'A':
			case 'D':
				goto skip;
			default:
				fprintf(stderr, "pcdisk: bad option '%c'\n", *p);
				return 1;
			}
		argv++;
		argc--;
	}
skip:
	if (argc > 2 && !strcmp(argv[1], "-D"))
	{
		devname = argv[2];
		argv += 2;
		argc -= 2;
	}
	
	if (argc > 2 && !strcmp(argv[1], "-A"))
	{
		actn  = atoi(argv[2]);
		argv += 2;
		argc -= 2;
		flag  = 1;
	}
	
	if (argc > 1)
		size = atoi(argv[1]);
	
	if (!flag)
	{
		fputs("pcdisk [-Xabcdgnp] [-D disk] [-A partition] [size]\n", stderr);
		return 1;
	}
	
	opendev();
	if (bigX)
		clear();
	if (dflag)
		dpart();
	if (cflag)
		cpart();
	if (aflag)
		sactiv();
	if (actn >= 0)
		sactn();
	if (pflag)
		prtab();
	if (gflag)
		printf("chs = %i,%i,%i\n", c, h, s);
	if (bflag)
		setipl();
	if (!nflag)
		save();
	
	return xit;
}
