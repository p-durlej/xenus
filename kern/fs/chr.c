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
#include <xenus/printf.h>
#include <xenus/umem.h>
#include <xenus/fs.h>
#include <sys/stat.h>
#include <errno.h>

struct chr_driver chr_driver[MAXCDEVS];

void ttydev_init(void);
void condev_init(void);
void procdev_init(void);
void memdev_init(void);
void rfd_init(void);
void rhd_init(void);

int chr_open(struct inode *ino, int nodelay)
{
	dev_t dev = ino->d.rdev;
	unsigned int i;
	
	i = major(dev);
	if (i >= MAXCDEVS)
	{
		printf("open chr bad %i,%i\n", major(dev), minor(dev));
		return ENXIO;
	}
	
	if (!chr_driver[i].open)
		return 0;
	
	return chr_driver[i].open(ino, nodelay);
}

int chr_close(struct inode *ino)
{
	dev_t dev = ino->d.rdev;
	unsigned int i;
	
	i = major(dev);
	if (i >= MAXCDEVS)
	{
		printf("close chr bad %i,%i\n", major(dev), minor(dev));
		return ENXIO;
	}
	
	if (!chr_driver[i].close)
		return 0;
	
	return chr_driver[i].close(ino);
}

int chr_read(struct rwreq *req)
{
	dev_t dev = req->ino->d.rdev;
	unsigned int i;
	
	i = major(dev);
	if (i >= MAXCDEVS)
	{
		printf("read chr bad %i,%i\n", major(dev), minor(dev));
		return ENXIO;
	}
	
	if (!chr_driver[i].read)
		return ENODEV;
	
	return chr_driver[i].read(req);
}

int chr_write(struct rwreq *req)
{
	dev_t dev = req->ino->d.rdev;
	unsigned int i;
	
	i = major(dev);
	if (i >= MAXCDEVS)
	{
		printf("write chr bad %i,%i\n", major(dev), minor(dev));
		return ENXIO;
	}
	
	if (!chr_driver[i].write)
		return ENODEV;
	
	return chr_driver[i].write(req);
}

int chr_ioctl(struct inode *ino, int cmd, void *ptr)
{
	dev_t dev = ino->d.rdev;
	unsigned int i;
	
	i = major(dev);
	if (i >= MAXCDEVS)
	{
		printf("ioctl chr bad %i,%i\n", major(dev), minor(dev));
		return ENXIO;
	}
	
	if (!chr_driver[i].ioctl)
		return ENOTTY;
	
	return chr_driver[i].ioctl(ino, cmd, ptr);
}

int chr_ttyname(struct inode *ino, char *buf)
{
	dev_t dev = ino->d.rdev;
	unsigned int i;
	
	i = major(dev);
	if (i >= MAXCDEVS)
	{
		printf("ttyname chr bad %i,%i\n", major(dev), minor(dev));
		return ENXIO;
	}
	
	if (!chr_driver[i].ttyname)
		return ENOTTY;
	
	return chr_driver[i].ttyname(ino, buf);
}

int null_read(struct rwreq *req)
{
	req->count = 0;
	return 0;
}

int null_write(struct rwreq *req)
{
	return 0;
}

int zero_read(struct rwreq *req)
{
	return uset(req->buf, 0, req->count);
}

void chr_init()
{
	chr_driver[NUL_DEVN].read   = null_read;
	chr_driver[NUL_DEVN].write  = null_write;
	
	chr_driver[ZERO_DEVN].read  = zero_read;
	chr_driver[ZERO_DEVN].write = null_write;
	
	ttydev_init();
	condev_init();
	procdev_init();
	memdev_init();
	rfd_init();
	rhd_init();
}
