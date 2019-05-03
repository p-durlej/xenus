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
#include <xenus/process.h>
#include <xenus/syscall.h>
#include <xenus/config.h>
#include <xenus/panic.h>
#include <xenus/page.h>
#include <xenus/exec.h>
#include <xenus/intr.h>
#include <xenus/umem.h>
#include <xenus/ualloc.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

struct process *pact[MAXPROCS];
struct process proc[MAXPROCS];
int sched_next;
int resched;
int npact = 1;
volatile int nidle;

struct process *curr = proc;
struct process *init = proc;

void user_mode(unsigned stack, unsigned start, unsigned args);
int do_exec(char *name, char *arg, char *env);

int sendsig(struct process *p, unsigned int sig);
int sendusig(struct process *p, unsigned int sig);
int sendksig(struct process *p, unsigned int sig);
void do_exit(int status);
void sched();
void updatecpu();
void proc_init();
void pmd(int sig);

static char ksig[NSIG] =
{
	[SIGQUIT]	= 1,
	[SIGILL]	= 1,
	[SIGTRAP]	= 1,
	[SIGABRT]	= 1,
	[SIGEMT]	= 1,
	[SIGBUS]	= 1,
	[SIGFPE]	= 1,
	[SIGSEGV]	= 1,
	[SIGHUP]	= 1,
	[SIGINT]	= 1,
	[SIGKILL]	= 1,
	[SIGPIPE]	= 1,
	[SIGTERM]	= 1,
	[SIGALRM]	= 1,
};

static char csig[NSIG] =
{
	[SIGQUIT]	= 1,
	[SIGILL]	= 1,
	[SIGTRAP]	= 1,
	[SIGABRT]	= 1,
	[SIGEMT]	= 1,
	[SIGBUS]	= 1,
	[SIGFPE]	= 1,
	[SIGSEGV]	= 1,
};

int sendsig(struct process *p, unsigned int sig)
{
	int flag = 1 << (sig - 1);
	
	if (!sig)
		return 0;
	if (sig >= NSIG)
		return EINVAL;
	if (!ksig[sig] && !(curr->sig_usr & flag))
		return 0;
	intr_disable();
	p->sig |= flag;
	p->sig &= ~p->sig_ign;
	intr_enable();
	return 0;
}

int sendusig(struct process *p, unsigned int sig)
{
	if (curr->euid != p->euid && curr->euid)
		return EPERM;
	return sendsig(p, sig);
}

int sendksig(struct process *p, unsigned int sig)
{
	p->sig_ign = 0;
	p->sig_usr = 0;
	return sendsig(p, sig);
}

void pushsig1(struct intr_regs *r, int nr)
{
	unsigned a[] =
	{
		curr->sigret1,
		nr,
		r->eip,
	};
	int w;
	int i;
	
	r->esp -= sizeof a;
	if (tucpy((void *)r->esp, a, sizeof a))
		pmd(SIGBUS);
	r->eip = curr->sig_proc[nr - 1];
}

void pushsig(struct intr_regs *r)
{
	unsigned int sig;
	unsigned int usr;
	int i;
	
	intr_disable();
	sig = curr->sig;
	curr->sig = 0;
	intr_enable();
	sig &= ~curr->sig_ign;
	usr = sig & curr->sig_usr;
	
	if (!sig)
		return;
	
	sig &= ~usr;
	
	if (usr)
	{
		unsigned a[] =
		{
			r->edi,
			r->esi,
			r->ebp,
			r->esp,
			r->ebx,
			r->edx,
			r->ecx,
			r->eax,
			r->eflags,
			r->eip,
			r->cs,
		};
		
		r->eip  = curr->sigret;
		r->cs	= USER_CS;
		r->esp -= sizeof a;
		if (tucpy((void *)r->esp, a, sizeof a))
			pmd(SIGBUS);
		
		curr->sig_usr &= ~usr;
		for (i = 1; i < NSIG; i++, usr >>= 1)
			if (usr & 1)
				pushsig1(r, i);
	}
	
	for (i = 1; i < NSIG; i++)
	{
		if (sig & 1)
		{
			if (csig[i])
				pmd(i);
			if (ksig[i])
				do_exit(i);
		}
		
		sig >>= 1;
	}
}

void do_exit(int status)
{
	struct process *p;
	int i;
	
	// printf("pid %i comm %s exit %i\n", curr->pid, curr->comm, status);
	if (curr == init)
	{
		printf("exit 0x%x\n", status);
		panic("init exited");
	}
	
	for (i = 0; i < MAXFDS; i++)
		if (curr->fd[i].file)
			fd_put(i);
	
	inode_put(curr->tty);
	inode_put(curr->cwd);
	inode_put(curr->root);
	
	for (i = 0; i < npact; i++)
	{
		p = pact[i];
		
		if (!p->pid)
			continue;
		if (p->parent == curr)
		{
			p->parent = init;
			if (p->exited)
				sendsig(init, SIGCHLD);
		}
		
		if (p->psid == curr->pid)
			p->psid = 0;
	}
	
	curr->parent->times.tms_cutime += curr->times.tms_utime;
	intr_disable();
	curr->parent->times.tms_cstime += curr->times.tms_stime;
	intr_enable();
	
	sendsig(curr->parent, SIGCHLD);
	
	curr->exit_status = status;
	curr->exited = 1;
	pcfree();
	wakeup();
	sched();
}

void sched()
{
	static struct process none;
	
	struct process *p;
	
	resched = 0;
	if (setjmp(curr->kstate))
		return;
	curr = &none;
	
	while (nidle >= npact)
		asm volatile ("hlt");
	
	for (;;)
	{
		if (sched_next >= npact)
			sched_next = 0;
		p = pact[sched_next++];
		
		p->time_slice += TIME_SLICE;
		if (p->time_slice > MAX_TIME_SLICE)
			p->time_slice = MAX_TIME_SLICE;
		
		if (!p->pid)
			continue;
		if (p->exited)
			continue;
		if (p->time_slice > 0)
			break;
	}
	
	curr = p;
	updatecpu();
	clock_chkalarm();
	longjmp(curr->kstate, 1);
}

void wakeup(void)
{
	nidle = 0;
}

void idle(void)
{
	if (dosync)
	{
		dosync = 0;
		psync();
	}
	nidle++;
	sched();
}

static void suseg(char *sd, char type, unsigned base, unsigned limit)
{
	sd[0] = limit;
	sd[1] = limit >> 8;
	sd[2] = base;
	sd[3] = base >>  8;
	sd[4] = base >> 16;
	sd[5] = type;
	sd[6] = (limit >> 16) | 0xc0;
	sd[7] = base >> 24;
}

void updatecpu()
{
	extern char *kstktop;
	
	pg_dir[USER_BASE >> 22] = (unsigned)curr->ptab | 7;
	pg_update();
	
	kstktop = curr->kstk + KSTK_SIZE;
}

void proc_init()
{
	extern char ucs_desc[];
	extern char uds_desc[];
	extern char scs_desc[];
	extern char sds_desc[];
	
	static struct inode cinode;
	static struct super csuper;
	
	struct intr_regs noregs;
	struct file *cfile;
	int i;
	
	suseg(ucs_desc, 0xfa, USER_BASE, (USER_SIZE >> 12) - 1);
	suseg(uds_desc, 0xf2, USER_BASE, (USER_SIZE >> 12) - 1);
	
	suseg(scs_desc, 0xfa, USER_BASE + USER_SIZE / 2, ((USER_SIZE / 2) >> 12) - 1);
	suseg(sds_desc, 0xf2, USER_BASE,		 ((USER_SIZE / 2) >> 12) - 1);
	
	uregs = &noregs;
	
	curr->pid	= 1;
	curr->psid	= 1;
	curr->parent	= curr;
	curr->kstk	= (void *)KSTK_BASE;
	curr->ptab	= zpalloc();
	
	if (!curr->ptab)
		panic("proc ptab");
	
	pact[0] = curr;
	
	csuper.dev    = -1U;
	cinode.sb     = &csuper;
	cinode.refcnt = 100;
	cinode.d.rdev = makedev(CON_DEVN, 0);
	cinode.d.mode = S_IFCHR;
	
	if (file_get(&cfile))
		panic("cfile");
	cfile->read   = chr_read;
	cfile->write  = chr_write;
	cfile->ioctl  = chr_ioctl;
	cfile->ino    = &cinode;
	cfile->refcnt = 3;
	cfile->flags  = 2; // O_RDWR
	
	for (i = 0; i < 3; i++)
		curr->fd[i].file = cfile;
	curr->tty = &cinode;
	
	if (do_exec("/bin/init", "/bin/init\377", ""))
		panic("can't exec init");
	user_mode(uregs->esp, uregs->eip, uregs->ebx);
}

void pmd(int sig)
{
	/* do_exit(sig | 128); */
	do_exit(sig);
}

int sys__exit(int status)
{
	do_exit((status & 0xff) << 8);
	return 0;
}

void *sys_signal(int signum, void *handler)
{
	void *prev = SIG_DFL;
	
	if (signum == SIGKILL)
		return SIG_DFL;
	
	if (signum < 1 || signum >= NSIG)
	{
		uerr(EINVAL);
		return SIG_ERR;
	}
	signum--;
	
	prev = (void *)curr->sig_proc[signum];
	curr->sig_proc[signum] = (unsigned)handler;
	
	switch ((int)handler)
	{
	case (int)SIG_DFL:
		curr->sig_ign &= ~(1 << signum);
		curr->sig_usr &= ~(1 << signum);
		break;
	case (int)SIG_IGN:
		curr->sig_ign |=   1 << signum;
		curr->sig_usr &= ~(1 << signum);
		break;
	default:
		curr->sig_usr |=   1 << signum;
		curr->sig_ign &= ~(1 << signum);
	}
	
	return prev;
}

int sys_kill(pid_t pid, int sig)
{
	struct process *p;
	int err = ESRCH;
	int sent = 0;
	int i;
	
	if (!pid)
	{
		err = sendusig(curr, sig);
		goto fini;
	}
	
	if (pid == -1)
	{
		for (i = 0; i < npact; i++)
		{
			p = pact[i];
			
			if (!p->pid || p == curr || p == init)
				continue;
			
			err = sendusig(p, sig);
			if (!err)
				sent = 1;
		}
		goto fini;
	}
	
	for (i = 0; i < npact; i++)
	{
		p = pact[i];
		
		if (p->pid == pid || p->psid == -pid)
		{
			err = sendusig(p, sig);
			break;
		}
	}
	
fini:
	if (err && !sent)
	{
		uerr(err);
		return -1;
	}
	return 0;
}

int sys__killu(uid_t ruid, int sig)
{
	struct process *p;
	int i;
	
	if (curr->euid)
	{
		uerr(EPERM);
		return -1;
	}
	
	for (i = 0; i < npact; i++)
	{
		p = pact[i];
		
		if (p->ruid == ruid)
			sendusig(p, sig);
	}
	return 0;
}

#if CDEBUGPROC
void cdebugproc()
{
	struct process *p;
	int i;
	
	printf("pact\n");
	for (i = 0; i < npact; i++)
	{
		p = pact[i];
		printf("i %i pid %i proc %p parent %p comm %s\n",
			i, p->pid, p, p->parent, p->comm);
	}
	
	printf("proc\n");
	for (i = 0; i < MAXPROCS; i++)
	{
		p = &proc[i];
		if (!p->pid)
			continue;
		printf("i %i pid %i proc %p parent %p comm %s\n",
			i, p->pid, p, p->parent, p->comm);
	}
}
#endif
