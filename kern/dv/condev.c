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

#include <xenus/console.h>
#include <xenus/process.h>
#include <xenus/config.h>
#include <xenus/ttyld.h>
#include <xenus/umem.h>
#include <xenus/fs.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>

#define VGASZ	(PCCON_NLIN * PCCON_NCOL * 2)

struct tty con_ttyld;
static int cd_attr0;
static int cd_attr;

int con_ttyname(struct inode *ino, char *buf)
{
	strcpy(buf, "/dev/console");
	return 0;
}

int con_ioctl(struct inode *ino, int cmd, void *ptr)
{
	return ttyld_ioctl(&con_ttyld, cmd, ptr);
}

int con_read(struct rwreq *req)
{
	return ttyld_read(&con_ttyld, req);
}

int con_write(struct rwreq *req)
{
	return ttyld_write(&con_ttyld, req);
}

int con_cread(struct tty *tty, char *c, int nodelay)
{
	while (!con_kbhit())
	{
		if (nodelay)
			return EAGAIN;
		if (curr->sig)
			return EINTR;
		idle();
	}
	
	*c = con_getch();
	return 0;
}

int con_cwrite(struct tty *tty, char c, int nodelay)
{
	static int ctrl = 0;
	
	switch (ctrl)
	{
	case 1:
		switch (c)
		{
		case 'u':
			memmove(con_fbuf, con_fbuf + PCCON_NCOL, VGASZ - PCCON_NCOL * 2);
			break;
		case 'd':
			memmove(con_fbuf + PCCON_NCOL, con_fbuf, VGASZ - PCCON_NCOL * 2);
			break;
		case 'p':
			ctrl = 2;
			return 0;
		case 'h':
			con_x = con_y = 0;
			con_setcursor();
			break;
		case 'i':
			cd_attr  = (cd_attr0 & 0x0f00) << 4;
			cd_attr |= (cd_attr0 & 0xf000) >> 4;
			break;
		case 'I':
			cd_attr = cd_attr0;
			break;
		default:
			con_putca(27, cd_attr);
			con_putca(c,  cd_attr);
		}
		ctrl = 0;
		break;
	case 2:
		con_x = (unsigned)c & 255;
		ctrl++;
		break;
	case 3:
		con_y = (unsigned)c & 255;
		con_setcursor();
		ctrl = 0;
		break;
	default:
		if (c == '\033')
			ctrl = 1;
		else
			con_putca(c, cd_attr);
	}
	return 0;
}

int con_speed(struct tty *tty, int speed)
{
	return speed;
}

int vga_read(struct rwreq *req)
{
	int err;
	
	if (req->start >= VGASZ)
		return EFAULT;
	if (req->start + req->count > VGASZ)
		return EFAULT;
	err = tucpy(req->buf, (char *)con_fbuf + req->start, req->count);
	if (err)
		return err;
	req->start += req->count;
	return 0;
}

int vga_write(struct rwreq *req)
{
	int err;
	
	if (req->start >= VGASZ)
		return EFAULT;
	if (req->start + req->count > VGASZ)
		return EFAULT;
	err = fucpy((char *)con_fbuf + req->start, req->buf, req->count);
	if (err)
		return err;
	req->start += req->count;
	return 0;
}

void condev_init()
{
	con_ttyld.dev	= makedev(CON_DEVN, 0);
	con_ttyld.read	= con_cread;
	con_ttyld.write	= con_cwrite;
	con_ttyld.speed	= con_speed;
	ttyld_init(&con_ttyld);
	ttyld_carrier(&con_ttyld, 1);
	
	chr_driver[CON_DEVN].ttyname	= con_ttyname;
	chr_driver[CON_DEVN].read	= con_read;
	chr_driver[CON_DEVN].write	= con_write;
	chr_driver[CON_DEVN].ioctl	= con_ioctl;
	chr_driver[VGA_DEVN].read	= vga_read;
	chr_driver[VGA_DEVN].write	= vga_write;
	
	cd_attr0 = cd_attr = con_attr;
}
