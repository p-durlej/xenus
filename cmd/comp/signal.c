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

#include <xenus/selector.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include "mnxcompat.h"

typedef void mnx_sighandler(int nr);

int sig_m2x[] =
{
	[0] = 0,	// unused
	[1] = SIGHUP,
	[2] = SIGINT,
	[3] = SIGQUIT,
	[4] = SIGILL,
	[5] = SIGTRAP,
	[6] = SIGABRT,
	[7] = 0,	// unused
	[8] = SIGFPE,
	[9] = SIGKILL,
	[10] = SIGUSR1,
	[11] = SIGSEGV,
	[12] = SIGUSR2,
	[13] = SIGPIPE,
	[14] = SIGALRM,
	[15] = SIGTERM,
	[16] = 0,	// unused
	[17] = SIGCHLD,
	[18] = 0,	// SIGCONT
	[19] = 0,	// SIGSTOP
	[20] = 0,	// SIGTSTP
	[21] = 0,	// SIGTTIN
	[22] = 0,	// SIGTTOU
	[23] = 0,	// SIGWINCH
};

int sig_x2m[NSIG];

struct mnx_sigact
{
	mnx_sighandler *handler;
	unsigned long mask;
	int flags;
} mnx_sigact[sizeof sig_m2x / sizeof *sig_m2x];

static void mnx_sighand(int nr)
{
	int msig = sig_x2m[nr];
	void (*proc)(int nr) = mnx_sigact[msig].handler;
	
#if MNXDEBUG
//	mprintf("mnx sigh proc %p nsig %i msig %i\n", proc, nr, msig);
#endif
	if (cs != USER_CS)
		callsig(proc, msig);
	else
		proc(msig);
}

int mnx_sigaction(struct message *msg)
{
	struct mnx_sigact *nact = msg->m1.p1, *oact = msg->m1.p2;
	unsigned msig = msg->m1.i2;
	unsigned nsig;
	
	if (msig >= sizeof sig_m2x / sizeof *sig_m2x)
	{
		errno = EINVAL;
		return -1;
	}
	nsig = sig_m2x[msig];
	
	*oact = mnx_sigact[msig];
	mnx_sigact[msig] = *nact;
	
#if MNXDEBUG
//	mprintf("mnx sigact proc %p nsig %i msig %i\n", nact->handler, nsig, msig);
#endif
	if ((unsigned)nact->handler < 2)
	{
		signal(nsig, nact->handler);
		return 0;
	}
	
	signal(nsig, mnx_sighand);
	return 0;
}

int mnx_sigsuspend(struct message *msg)
{
	return pause(); /* XXX */
}

int mnx_sigpending(struct message *msg)
{
	msg->m2.l1 = 0; /* XXX */
	return 0;
}

int mnx_sigprocmask(struct message *msg)
{
	return 0; /* XXX */
}

int mnx_signal(struct message *msg)
{
	void *h  = msg->m6.sigh;
	int msig = msg->m6.i1;
	int nsig;
	
	if (msig >= sizeof sig_m2x / sizeof *sig_m2x)
	{
		errno = EINVAL;
		return -1;
	}
	nsig = sig_m2x[msig];
	
	mnx_sigact[msig].handler = h;
	mnx_sigact[msig].mask	 = 0;
	mnx_sigact[msig].flags	 = 0;
	
	if ((unsigned)h < 2)
	{
		signal(nsig, h);
		return 0;
	}
	
	signal(nsig, mnx_sighand);
	return 0;
}

void siginit(void)
{
	int i;
	
	for (i = 1; i < sizeof sig_m2x / sizeof *sig_m2x; i++)
		if (sig_m2x[i])
			sig_x2m[sig_m2x[i]] = i;
}
