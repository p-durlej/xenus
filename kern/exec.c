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
#include <xenus/printf.h>
#include <xenus/config.h>
#include <xenus/ualloc.h>
#include <xenus/umem.h>
#include <xenus/exec.h>
#include <xenus/page.h>
#include <xenus/fs.h>
#include <limits.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

extern char sigret, sigret1, sigrete;

void updatecpu();

int loadhdr(struct inode *ino, struct exehdr *hdr)
{
	struct blk_buffer *b;
	int err;
	
	err = bmget(ino, 0, &b);
	if (err)
		return err;
	if (!b)
	{
		memset(hdr, 0, sizeof *hdr);
		return 0;
	}
	memcpy(hdr, b->data, sizeof *hdr);
	blk_put(b);
	return 0;
}

int loadbin(struct inode *ino, unsigned base)
{
	unsigned size = ino->d.size;
	unsigned off = 0;
	unsigned l;
	blk_t blk;
	int err;
	
	while (off != size)
	{
		if (size - off < BLK_SIZE)
			l = size - off;
		else
			l = BLK_SIZE;
		
		blk = off / BLK_SIZE;
		err = bmap(ino, &blk);
		if (err)
			return err;
		if (!blk)
			err = uset((char *)base + off, 0, l);
		else
			err = blk_upread(ino->sb->dev, blk, 0, l, (char *)base + off);
		if (err)
			return err;
		
		off += l;
	}
	
	return 0;
}

int do_exec(char *name, char *arg, char *env)
{
	unsigned arga, enva, siga, stka, nama;
	char *compat = NULL;
	char *oname = NULL;
	char *narg = NULL;
	struct exehdr hdr;
	struct inode *ino;
	unsigned *pt = curr->ptab;
	struct rwreq rw;
	int err;
	int i;
	
again:
	err = dir_traverse(name, 0, &ino);
	if (err)
	{
		if (err == ENOENT && oname)
			return ENOEXEC;
		return err;
	}
	
	if (!S_ISREG(ino->d.mode) || !(ino->d.mode & 0111))
	{
		inode_put(ino);
		return EACCES;
	}
	
	err = inode_chkperm(ino, X_OK);
	if (err)
	{
		inode_put(ino);
		return err;
	}
	
	err = loadhdr(ino, &hdr);
	if (err)
	{
		inode_put(ino);
		return err;
	}
	
	if (memcmp(hdr.magic, "XENUS386", 8))
	{
		inode_put(ino);
		
		for (i = 0; i < sizeof hdr.magic && hdr.magic[i]; i++)
		{
			unsigned c = hdr.magic[i];
			
			if (c >= 0x20 && c < 0x80)
				continue;
			if (c == '\n' || c == '\t')
				continue;
			break;
		}
		if (i >= sizeof hdr.magic || !hdr.magic[i])
		{
			if (narg)
			{
				pfree(narg);
				return ENOEXEC;
			}
			
			if (strlen(arg) + sizeof SHELL > ARG_MAX)
				return E2BIG;
			
			arg = strchr(arg, '\377') + 1;
			if (arg == (void *)1)
				return EINVAL;
			
			narg = palloc();
			if (!narg)
				return ENOMEM;
			
			strcpy(narg, SHELL "\377");
			strcat(narg, name);
			strcat(narg, "\377");
			strcat(narg, arg);
			arg = narg;
			name = SHELL;
			goto again;
		}
		
		if (hdr.magic[0] == 1 && hdr.magic[1] == 3)
			compat = "/usr/compat/minix";
		
		if (!compat)
		{
			pfree(narg);
			return ENOEXEC;
		}
		
		if (oname)
		{
			printf("bad %s\n", name);
			pfree(narg);
			return ENOEXEC;
		}
		oname = name;
		name = compat;
		goto again;
	}
	
	if (hdr.base >= USER_SIZE)
	{
		inode_put(ino);
		pfree(narg);
		return ENOEXEC;
	}
	
	if (hdr.end > USER_SIZE || hdr.stack > USER_SIZE || hdr.end + hdr.stack > USER_SIZE)
	{
		inode_put(ino);
		pfree(narg);
		return ENOEXEC;
	}
	
	siga  = USER_SIZE - (&sigrete - &sigret);
	nama  = siga;
	if (oname)
		nama -= strlen(oname) + 1;
	enva  = nama - strlen(env) - 1;
	arga  = enva - strlen(arg) - 1;
	stka  = arga - hdr.stack;
	stka &= ~7;
	
	for (i = 0; i < 1024; i++)
		if (pt[i])
		{
			pg_free(pt[i] >> 12);
			pt[i] = 0;
		}
	pg_update();
	
	curr->size	= 0;
	curr->errno	= (void *)hdr.errno;
	curr->sigret	= siga;
	curr->sigret1	= siga + (&sigret1 - &sigret);
	curr->sig_usr	= 0;
	curr->brk	= hdr.end;
	curr->stk	= stka;
	curr->astk	= USER_SIZE;
	curr->base	= hdr.base;
	curr->end	= hdr.end;
	curr->compat	= 0;
	
	if (hdr.base)
		curr->brk = 16;
	
	err = pcalloc();
	if (err)
	{
		sendksig(curr, SIGABRT);
		inode_put(ino);
		pfree(narg);
		return 0;
	}
	
	if (S_ISUID & ino->d.mode)
		curr->euid = ino->d.uid;
	if (S_ISGID & ino->d.mode)
		curr->egid = ino->d.gid;
	
	updatecpu();
	err = loadbin(ino, hdr.base);
	inode_put(ino);
	if (err)
	{
		sendksig(curr, SIGABRT);
		pfree(narg);
		return 0;
	}
	
	uregs->eip	= hdr.start;
	uregs->eax	= 0;
	uregs->ebx	= arga;
	uregs->ecx	= 0;
	uregs->esi	= enva;
	uregs->edi	= nama;
	uregs->ebp	= 0;
	uregs->cs	= USER_CS;
	uregs->ds	= USER_DS;
	uregs->es	= USER_DS;
	uregs->fs	= USER_DS;
	uregs->gs	= USER_DS;
	uregs->ss	= USER_DS;
	uregs->esp	= arga;
	uregs->eflags  &= 0xffc0822a;
	
	if (oname)
	{
		strncpy(curr->comm, basename(oname), sizeof(curr->comm));
		tucpy((void *)nama, oname, strlen(oname));
		curr->cpname = name;
	}
	else
	{
		strncpy(curr->comm, basename(name), sizeof(curr->comm));
		uregs->edi = 0;
		curr->cpname = NULL;
	}
	
	tucpy((void *)arga, arg, strlen(arg));
	tucpy((void *)enva, env, strlen(env));
	tucpy((void *)siga, &sigret, &sigrete - &sigret);
	fd_cloexec();
	return 0;
}

int sys__exec(char *name, char *arg, char *env)
{
	char *lname = palloc();
	char *larg = palloc();
	char *lenv = palloc();
	int err;
	
	if (!lname || !larg || !lenv)
		goto fail;
	
	err = fustr(lname, name, PATH_MAX);
	if (err)
		goto fail;
	
	err = fustr(larg, arg, ARG_MAX);
	if (err)
		goto fail;
	
	err = fustr(lenv, env, ARG_MAX);
	if (err)
		goto fail;
	
	err = do_exec(lname, larg, lenv);
fail:
	pfree(lname);
	pfree(larg);
	pfree(lenv);
	if (err)
	{
		uerr(err);
		return -1;
	}
	return 0;
}
