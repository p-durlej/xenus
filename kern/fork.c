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
#include <xenus/syscall.h>
#include <xenus/config.h>
#include <xenus/umem.h>
#include <xenus/fs.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

pid_t allocpid()
{
	static pid_t nextpid = 1;
	int i;
	
	nextpid++;
	if (nextpid > 9999)
		nextpid = 2;
	
restart:
	for (i = 0; i < npact; i++)
		if (pact[i]->pid == nextpid || pact[i]->psid == nextpid)
		{
			nextpid++;
			goto restart;
		}
	
	return nextpid;
}

pid_t sys_fork()
{
	struct intr_regs *nr;
	int cnt = 0;
	int i;
	int n;
	
	void asm_afterfork();
	
	for (i = 0; i < npact; i++)
		if (pact[i]->ruid == curr->ruid)
			cnt++;
	if (cnt >= MAXUPROC && curr->euid)
	{
		uerr(EAGAIN);
		return -1;
	}
	
	for (i = 0; i < MAXPROCS; i++)
		if (!proc[i].pid)
			break;
	if (i >= MAXPROCS)
	{
		uerr(EAGAIN);
		return -1;
	}
	
	proc[i]		= *curr;
	proc[i].parent	= curr;
	proc[i].kstk	= malloc(KSTK_SIZE);
	proc[i].base	= (unsigned)malloc(curr->size);
	if (!proc[i].kstk || !proc[i].base)
	{
		memset(&proc[i], 0, sizeof proc[i]);
		free(proc[i].kstk);
		free((void *)proc[i].base);
		uerr(ENOMEM);
		return -1;
	}
	memcpy((void *)proc[i].base, (void *)curr->base, curr->size);
	proc[i].pid = allocpid();
	for (n = 0; n < MAXFDS; n++)
		if (curr->fd[n].file)
			curr->fd[n].file->refcnt++;
	if (curr->tty)
		curr->tty->refcnt++;
	curr->cwd->refcnt++;
	curr->root->refcnt++;
	nr = (struct intr_regs *)(proc[i].kstk + KSTK_SIZE - sizeof(*uregs));
	proc[i].kstate->esp  = (u32_t)nr;
	proc[i].kstate->esp -= 4;
	proc[i].kstate->eip  = (u32_t)asm_afterfork;
	memcpy(nr, uregs, sizeof(*uregs));
	nr->eax	   = 0;
	nr->eflags = nr->esp;
	nr->esp	   = nr->ss;
	pact[npact++] = &proc[i];
	return proc[i].pid;
}

pid_t sys_wait(int *status)
{
	struct process *z = NULL;
	struct process *c = NULL;
	struct process *p;
	pid_t pid;
	int err;
	int zi;
	int i;
	
restart:
	for (i = 0; i < npact; i++)
	{
		p = pact[i];
		
		if (p->pid == 1 || !p->pid || p->parent != curr)
			continue;
		c = p;
		if (c->exited)
		{
			zi = i;
			z = c;
		}
	}
	
	if (!c)
	{
		uerr(ECHILD);
		return -1;
	}
	
	if (!z)
	{
		idle();
		if (curr->sig)
		{
			uerr(EINTR);
			return -1;
		}
		goto restart;
	}
	
	err = tucpy(status, &z->exit_status, sizeof(int));
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	pid = z->pid;
	free(z->kstk);
	memset(z, 0, sizeof *z);
	pact[zi] = pact[--npact];
	
	return pid;
}
