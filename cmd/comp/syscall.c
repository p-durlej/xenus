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
#include <sys/times.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

#include "mnxcompat.h"

typedef int mnx_syscall(struct message *msg);

int trace;

static int mnx_exit(struct message *msg)
{
	_exit(msg->m1.i1);
	return 0; // XXX silence a warning
}

static int mnx_fork(struct message *msg)
{
	return fork();
}

static int cstatus(int st)
{
	int msig = sig_x2m[st & 255];
	
	st &= 0xff00;
	st |= msig;
	
	return st;
}

static int mnx_wait(struct message *msg)
{
	pid_t pid;
	int st;
	
	pid = wait(&st);
	if (pid > 0)
		msg->m2.i1 = cstatus(st);
	return pid;
}

static int mnx_waitpid(struct message *msg)
{
	pid_t pid = msg->m1.i1;
	int opt = msg->m1.i2;
	int st;
	
#if MNXDEBUG
//	mprintf("mnx waitpid pid %i opt 0x%x\n", pid, opt); // XXX
#endif
	pid = waitpid(pid, &st, opt);
	if (pid > 0)
		msg->m2.i1 = cstatus(st);
	return pid;
}

static int mnx_brk(struct message *msg)
{
#if MNXDEBUG
//	mprintf("mnx brk %p\n", msg->m1.p1);
#endif
	if (brk(msg->m1.p1) < 0)
	{
		msg->m2.p1 = (void *)-1;
		return -1;
	}
	msg->m2.p1 = msg->m1.p1;
	return 0;
}

static int mnx_getuid(struct message *msg)
{
	msg->m2.i1 = geteuid();
	return getuid();
}

static int mnx_getgid(struct message *msg)
{
	msg->m2.i1 = getegid();
	return getgid();
}

static int mnx_umask(struct message *msg)
{
	return umask(msg->m1.i1);
}

static int mnx_kill(struct message *msg)
{
	return kill(msg->m1.i1, msg->m1.i2);
}

static int mnx_getpid(struct message *msg)
{
	msg->m2.i1 = getppid();
	return getpid();
}

static int mnx_time(struct message *msg)
{
	msg->m2.l1 = time(NULL);
	return 0;
}

static int mnx_exec(struct message *msg)
{
	unsigned *stack = msg->m1.p2;
	unsigned *p = stack;
	int ac = *p++ + 1;
	int ec;
	int i;
	char **av, **ev;
	char *obrk;
	
	obrk = sbrk(0);
	
	av = sbrk(ac * sizeof *av);
	memcpy(av, p, ac * sizeof *av);
	p += ac;
	
	for (ec = 0; p[ec]; ec++)
		;
	ev = sbrk(++ec * sizeof *ev);
	memcpy(ev, p, ec * sizeof *ev);
	
	for (i = 0; av[i]; i++)
		av[i] += (unsigned)stack;
	for (i = 0; ev[i]; i++)
		ev[i] += (unsigned)stack;
	
	execve(msg->m1.p1, av, ev);
	brk(obrk);
	return -1;
}

static int mnx_alarm(struct message *msg)
{
	return alarm(msg->m1.i1);
}

static int mnx_setuid(struct message *msg)
{
	return setuid(msg->m1.i1);
}

static int mnx_setgid(struct message *msg)
{
	return setgid(msg->m1.i1);
}

static int mnx_stime(struct message *msg)
{
	time_t t = msg->m2.l1;
	
	return stime(&t);
}

static int mnx_times(struct message *msg)
{
	struct tms tms;
	clock_t t;
	
	t = times(&tms);
	msg->m4.l1 = tms.tms_utime  * 60 / HZ;
	msg->m4.l2 = tms.tms_stime  * 60 / HZ;
	msg->m4.l3 = tms.tms_cutime * 60 / HZ;
	msg->m4.l4 = tms.tms_cstime * 60 / HZ;
	msg->m4.l5 = t * 60 / HZ;
	return 0;
}

static int mnx_setsid(struct message *msg)
{
	return setsid();
}

static int mnx_getpgrp(struct message *msg)
{
	return getpgrp();
}

static int mnx_nosys(struct message *msg)
{
	errno = ENOSYS;
	return -1;
}

static mnx_syscall *systab[] = {
	[ 1] = mnx_exit,
	[ 2] = mnx_fork,
	[ 3] = mnx_read,
	[ 4] = mnx_write,
	[ 5] = mnx_open,
	[ 6] = mnx_close,
	[ 7] = mnx_wait,
	[ 8] = mnx_creat,
	[ 9] = mnx_link,
	[10] = mnx_unlink,
	[11] = mnx_waitpid,
	[12] = mnx_chdir,
	[13] = mnx_time,
	[14] = mnx_mknod,
	[15] = mnx_chmod,
	[16] = mnx_chown,
	[17] = mnx_brk,
	[18] = mnx_stat,
	[19] = mnx_lseek,
	[20] = mnx_getpid,
	[21] = mnx_mount,
	[22] = mnx_umount,
	[23] = mnx_setuid,
	[24] = mnx_getuid,
	[25] = mnx_stime,
	[26] = mnx_nosys, /* ptrace */
	[27] = mnx_alarm,
	[28] = mnx_fstat,
	[29] = (void *)pause,
	[30] = mnx_utime,
	[33] = mnx_access,
	[36] = (void *)sync,
	[37] = mnx_kill,
	[39] = mnx_mkdir,
	[40] = mnx_rmdir,
	[41] = mnx_dup,
	[42] = mnx_pipe,
	[43] = mnx_times,
	[46] = mnx_setgid,
	[47] = mnx_getgid,
	[48] = mnx_signal,
	[54] = mnx_ioctl,
	[55] = mnx_fcntl,
	[59] = mnx_exec,
	[60] = mnx_umask,
	[61] = mnx_chroot,
	[62] = mnx_setsid,
	[63] = mnx_getpgrp,
	[71] = mnx_sigaction,
	[72] = mnx_sigsuspend,
	[73] = mnx_sigpending,
	[74] = mnx_sigprocmask,
	
	[100] = mnx_nosys, /* mysterious syscall invoked by the ACK */
};

int syscall(struct message *msg, unsigned eip, unsigned cs)
{
	unsigned nr = msg->type;
	mnx_syscall *proc;
	int r;
	
#if MNXDEBUG
	if (trace)
		mprintf("mnx %u at %04x:%08x\n", nr, cs, eip);
#endif
	
	if (nr >= sizeof systab / sizeof *systab)
		goto badsys;
	
	proc = systab[nr];
	if (!proc)
		goto badsys;
	
	r = proc(msg);
	if (r < 0)
		r = mnx_error(errno);
#if MNXDEBUG
	if (trace)
		mprintf("mnx result %i\n", r);
#endif
	msg->type = r;
	return 0;
badsys:
#if MNXDEBUG
	mprintf("mnx syscall %u\n", nr);
	abort(); // XXX
#endif
	msg->type = mnx_error(ENOSYS);
	return 0;
}
