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
#include <sys/mount.h>
#include <sys/times.h>
#include <sys/ftime.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <utime.h>
#include <time.h>

#include "v7compat.h"

extern int v7_getuid();
extern int v7_getgid();
extern int v7_getpid();
extern int v7_fork();
extern int v7_wait();
extern int v7_pipe();

#if V7DEBUG
extern int cursys;

int trace;

void v7_trace()
{
	if (trace)
		mprintf("v7 sys %i\n", cursys);
}
#endif

int v7_nosys()
{
#if V7DEBUG
	mprintf("v7 sys %i\n", cursys);
	abort();
#endif
	raise(SIGSYS);
	errno = ENOSYS;
	return -1;
}

static int v7_nop()
{
	return 0;
}

static int v7_times(int *t)
{
	struct tms tms;
	
	times(&tms);
	t[0] = tms.tms_utime  * 60 / HZ;
	t[1] = tms.tms_stime  * 60 / HZ;
	t[2] = tms.tms_cutime * 60 / HZ;
	t[3] = tms.tms_cstime * 60 / HZ;
	return 0;
}

static int v7_gtty(int fd, void *data)
{
	return v7_ioctl(fd, 0x7408, data);
}

static int v7_stty(int fd, void *data)
{
	return v7_ioctl(fd, 0x7409, data);
}

static int v7_execve(char *path, char **argv, char **envp)
{
	return execve(xbin(path), argv, envp);
}

static int v7_execv(char *path, char **argv)
{
	return execv(xbin(path), argv);
}

void *systab[64] =
{
	[ 0] = v7_nop,	 /* indir */
	[ 1] = _exit,
	[ 2] = v7_fork,
	[ 3] = v7_read,
	[ 4] = write,
	[ 5] = v7_open,
	[ 6] = close,
	[ 7] = v7_wait,
	[ 8] = v7_creat,
	[ 9] = v7_link,	 /* XXX */
	[10] = v7_unlink,/* XXX */
	[11] = v7_execv,
	[12] = v7_chdir,
	[13] = time,
	[14] = v7_mknod,
	[15] = v7_chmod,
	[16] = v7_chown,
	[17] = brk,
	[18] = v7_stat,
	[19] = lseek,
	[20] = v7_getpid,
	[21] = v7_mount,
	[22] = v7_nosys, /* XXX umount */
	[23] = setuid,
	[24] = v7_getuid,
	[25] = stime,
	[26] = v7_nosys, /* ptrace */
	[27] = alarm,
	[28] = v7_fstat,
	[29] = pause,
	[30] = utime,
	[31] = v7_stty,
	[32] = v7_gtty,
	[33] = v7_access,
	[34] = v7_nop,	 /* nice */
	[35] = ftime,
	[36] = sync,
	[37] = kill,
	[38] = v7_nop,	 /* UNUSED */
	[39] = v7_nop,	 /* UNUSED */
	[40] = v7_nosys, /* UNUSED */
	[41] = v7_dup,
	[42] = v7_pipe,
	[43] = v7_times,
	[44] = v7_nosys, /* profil */
	[45] = v7_nosys, /* UNUSED */
	[46] = setgid,
	[47] = v7_getgid,
	[48] = signal,
	[49] = v7_nosys, /* UNUSED */
	[50] = v7_nosys, /* UNUSED */
	[51] = v7_nosys, /* acct */
	[52] = v7_nosys, /* phys */
	[53] = v7_nop,	 /* lock */
	[54] = v7_ioctl,
	[55] = v7_nosys, /* UNUSED */
	[56] = v7_nosys, /* mpx */
	[57] = v7_nosys, /* UNUSED */
	[58] = v7_nosys, /* UNUSED */
	[59] = v7_execve,
	[60] = umask,
	[61] = v7_chroot,
	[62] = v7_nosys, /* x */
	[63] = v7_nosys, /* INTERNAL */
};

#if V7DEBUG
void sysinit(void)
{
	int i;
	
	for (i = 0; i < sizeof systab / sizeof *systab; i++)
		if (!systab[i])
		{
			mprintf("v7 no sys %i\n", i);
			systab[i] = v7_nosys;
		}
}
#endif
