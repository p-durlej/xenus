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

#include <xenus/process.h>
#include <xenus/config.h>
#include <xenus/fs.h>
#include <unistd.h>
#include <errno.h>

int tty_ioctl(struct inode *ino, int cmd, void *ptr)
{
	if (!curr->tty)
		return ENODEV;
	return chr_ioctl(curr->tty, cmd, ptr);
}

int tty_read(struct rwreq *req)
{
	int err;
	
	if (!curr->tty)
		return ENODEV;
	
	req->ino = curr->tty;
	return chr_read(req);
}

int tty_write(struct rwreq *req)
{
	int err;
	
	if (!curr->tty)
		return ENODEV;
	
	req->ino = curr->tty;
	return chr_write(req);
}

int tty_ttyname(struct inode *ino, char *buf)
{
	if (!curr->tty)
		return ENODEV;
	
	return chr_ttyname(curr->tty, buf);
}

void ttydev_init()
{
	chr_driver[TTY_DEVN].ttyname	= tty_ttyname;
	chr_driver[TTY_DEVN].read	= tty_read;
	chr_driver[TTY_DEVN].write	= tty_write;
	chr_driver[TTY_DEVN].ioctl	= tty_ioctl;
}
