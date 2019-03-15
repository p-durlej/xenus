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
#include <xenus/malloc.h>
#include <xenus/config.h>
#include <xenus/umem.h>
#include <xenus/exec.h>
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
		return ENOEXEC;
	memcpy(hdr, b->data, sizeof(struct exehdr));
	blk_put(b);
	if (memcmp(hdr->magic, "XENUS386", 8))
		return ENOEXEC;
	return 0;
}

int loadbin(struct inode *ino)
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
			err = uset((char *)off, 0, l);
		else
			err = blk_upread(ino->sb->dev, blk, 0, l, (char *)off);
		if (err)
			return err;
		
		off += l;
	}
	
	return 0;
}

int do_exec(char *name, char *arg, char *env)
{
	struct exehdr hdr;
	struct inode *ino;
	unsigned int size;
	struct rwreq rw;
	int err;
	
	err = dir_traverse(name, 0, &ino);
	if (err)
		return err;
	if (!S_ISREG(ino->d.mode))
	{
		inode_put(ino);
		return EPERM;
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
	size = hdr.end + hdr.extra + strlen(arg) + strlen(env) + 2 + (&sigrete - &sigret);
	if (size > 0x100000)
	{
		inode_put(ino);
		return ENOEXEC;
	}
	if (S_ISUID & ino->d.mode)
		curr->euid = ino->d.uid;
	if (S_ISGID & ino->d.mode)
		curr->egid = ino->d.gid;
	free((void *)curr->base);
	curr->base	= (unsigned int)malloc(size);
	curr->size	= size;
	curr->errno	= (void *)hdr.errno;
	curr->sigret	= hdr.end + hdr.extra + strlen(arg) + strlen(env) + 2;
	curr->sigret1	= curr->sigret + (&sigret1 - &sigret);
	curr->sig_usr	= 0;
	curr->brk	= (void *)hdr.end;
	if (!curr->base)
	{
		sendksig(curr, SIGABRT);
		inode_put(ino);
		return 0;
	}
	updatecpu();
	uset(NULL, 0, size);
	err = loadbin(ino);
	inode_put(ino);
	if (err)
	{
		sendksig(curr, SIGABRT);
		return 0;
	}
	uregs->eip	= hdr.start;
	uregs->eax	= 0;
	uregs->ebx	= hdr.end + hdr.extra;
	uregs->ecx	= 0;
	uregs->esi	= 0;
	uregs->edi	= 0;
	uregs->ebp	= 0;
	uregs->cs	= USER_CS;
	uregs->ds	= USER_DS;
	uregs->es	= USER_DS;
	uregs->fs	= USER_DS;
	uregs->gs	= USER_DS;
	uregs->ss	= USER_DS;
	uregs->esp	= uregs->ebx;
	uregs->eflags  &= 0xffc0822a;
	strncpy(curr->comm, basename(name), sizeof(curr->comm));
	strcpy((char *)curr->base + uregs->esp, arg);
	strcpy((char *)curr->base + uregs->esp + strlen(arg) + 1, env);
	memcpy((char *)curr->base + curr->sigret, &sigret, &sigrete - &sigret);
	fd_cloexec();
	return 0;
}

int sys__exec(char *name, char *arg, char *env)
{
	char *lname;
	char *larg;
	char *lenv;
	int err;
	
	err = usdup(&lname, name, PATH_MAX);
	if (err)
		goto fail;
	
	err = usdup(&larg, arg, ARG_MAX);
	if (err)
		goto fail;
	
	err = usdup(&lenv, env, ARG_MAX);
	if (err)
		goto fail;
	
	err = do_exec(lname, larg, lenv);
	if (err)
		goto fail;
fail:
	free(lname);
	free(larg);
	free(lenv);
	if (err)
	{
		uerr(err);
		return -1;
	}
	return 0;
}
