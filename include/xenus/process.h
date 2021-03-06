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

#include <xenus/clock.h>
#include <xenus/intr.h>
#include <xenus/fs.h>
#include <sys/types.h>
#include <sys/times.h>
#include <signal.h>
#include <setjmp.h>

struct process
{
	pid_t		pid;
	pid_t		psid;
	uid_t		euid;
	uid_t		ruid;
	gid_t		egid;
	gid_t		rgid;
	
	unsigned short	exit_status;
	char		exited;
	volatile int	time_slice;
	char		comm[NAME_MAX + 1];
	struct process *parent;
	volatile struct systime alarm;
	struct tms	times;
	
	blk_t		fslimit;
	mode_t		umask;
	char		linkmode;
	short		namlen;
	struct inode *	tty;
	struct inode *	cwd;
	struct inode *	root;
	struct fd	fd[MAXFDS];
	
	unsigned	sig_proc[NSIG];
	int		sig_ign;
	int		sig_usr;
	volatile int	sig;
	unsigned	sigret, sigret1;
	int *		errp;
	
	char *		kstk;
	jmp_buf		kstate;
	unsigned	size;
	unsigned *	ptab;
	
	unsigned	brk;
	unsigned	stk, astk;
	unsigned	base, end;
	unsigned	compat;
	char *		cpname;
	
	char		fpust[108];
};

extern struct process *pact[];
extern struct process proc[];
extern int npact;

extern struct process *curr;
extern struct process *init;
extern int resched;
extern int fpu;

int sendsig(struct process *p, unsigned int sig);
int sendusig(struct process *p, unsigned int sig);
int sendksig(struct process *p, unsigned int sig);
void pushsig(struct intr_regs *r);

void do_exit(int status);

void wakeup(void);
void sched(void);
void idle(void);
void pmd(int sig);

void fsave(void *st);
void fload(void *st);
