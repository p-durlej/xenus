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
#include <xenus/procdev.h>
#include <xenus/config.h>
#include <xenus/umem.h>
#include <xenus/fs.h>
#include <string.h>
#include <errno.h>

int proc_read(struct rwreq *req)
{
	struct procinfo pi;
	unsigned int i;
	int err;
	
	if (req->count != sizeof(pi))
		return EINVAL;
	i = req->start;
	
	if (i >= MAXPROCS)
	{
		req->count = 0;
		return 0;
	}
	
	pi.base		= proc[i].base;
	pi.size		= proc[i].size;
	pi.pid		= proc[i].pid;
	pi.psid		= proc[i].psid;
	pi.ppid		= proc[i].parent->pid;
	pi.euid		= proc[i].euid;
	pi.egid		= proc[i].egid;
	pi.ruid		= proc[i].ruid;
	pi.rgid		= proc[i].rgid;
	pi.time_slice	= proc[i].time_slice;
	if (proc[i].tty)
		pi.tty = proc[i].tty->d.rdev;
	else
		pi.tty = -1;
	memcpy(&pi.comm, &proc[i].comm, sizeof(pi.comm));
	pi.comm[sizeof(pi.comm) - 1] = 0;
	err = tucpy(req->buf, &pi, sizeof(pi));
	req->start++;
	return err;
}

int proc_write(struct rwreq *req)
{
	return EPERM;
}

void procdev_init()
{
	chr_driver[PROC_DEVN].read  = proc_read;
	chr_driver[PROC_DEVN].write = proc_write;
}
