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

#include <xenus/syscall.h>
#include <xenus/process.h>
#include <xenus/console.h>
#include <xenus/printf.h>
#include <xenus/clock.h>
#include <xenus/page.h>
#include <xenus/ualloc.h>
#include <xenus/umem.h>
#include <xenus/intr.h>
#include <sys/types.h>
#include <string.h>
#include <stddef.h>
#include <errno.h>

void *systab[]=
{
	sys__sysmesg,
	sys__exit,
	sys_brk,
	sys_getpid,
	sys_getppid,
	sys_getuid,
	sys_geteuid,
	sys_getgid,
	sys_getegid,
	sys_signal,
	sys_kill,
	sys_mknod,
	sys_open,
	sys_close,
	sys_read,
	sys_write,
	sys_lseek,
	sys_fstat,
	sys_fchmod,
	sys_fchown,
	sys_access,
	sys_chdir,
	sys_chroot,
	sys_unlink,
	sys_mkdir,
	sys_rmdir,
	sys_link,
	sys_rename,
	sys_ioctl,
	sys_fcntl,
	sys__exec,
	sys_fork,
	sys_wait,
	sys_pause,
	sys_time,
	sys_stime,
	sys_alarm,
	sys_setuid,
	sys_setgid,
	sys_setsid,
	sys_ttyname3,
	sys_statfs,
	sys_mount,
	sys_umount,
	sys_sync,
	sys_umask,
	sys__ctty,
	sys_pipe,
	sys__killf,
	sys_uname,
	sys__iopl,
	sys__mfree,
	sys_reboot,
	sys_stat,
	sys_chmod,
	sys_chown,
	sys_dup,
	sys_dup2,
	sys__uptime,
	sys_sbrk,
	sys__dmesg,
	sys_sleep,
	sys__killu,
	sys_utime,
	sys__newregion,
	sys__setcompat,
	sys_waitpid,
	sys_times,
};

struct intr_regs *uregs;

void uerr(int errno)
{
	tucpy(curr->errno, &errno, sizeof(errno));
}

void syscall(struct intr_regs *r)
{
	int (*p)(int arg0, int arg1, int arg2, int arg3);
	unsigned int nr = r->eax;
	int arg[4];
	
	uregs = r;
	
	if (fucpy(arg, (void *)(r->esp + 4), sizeof(arg)))
	{
		uerr(EFAULT);
		r->eax = -1;
		return;
	}
	
	if (nr >= sizeof(systab) / sizeof(void *))
	{
		uerr(ENOSYS);
		r->eax = -1;
		return;
	}
	
	p = systab[nr];
	r->eax = p(arg[0], arg[1], arg[2], arg[3]);
}

int sys__sysmesg(char *msg, unsigned int len)
{
	int err;
	char *p;
	
	if (curr->euid)
	{
		uerr(EPERM);
		return -1;
	}
	
	err = urchk(msg, len);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	p = msg + USER_BASE;
	while (len--)
		con_putc(*p++);
	return 0;
}

int sys_getpid(void)
{
	return curr->pid;
}

int sys_getppid(void)
{
	return curr->parent->pid;
}

int sys_getuid(void)
{
	return curr->ruid;
}

int sys_geteuid(void)
{
	return curr->euid;
}

int sys_getgid(void)
{
	return curr->rgid;
}

int sys_brk(void *addr)
{
	unsigned e = ((unsigned)addr + 4095) >> 12;
	unsigned *pt = curr->ptab;
	int i;
	
	if ((unsigned)addr > uregs->esp)
	{
		uerr(ENOMEM);
		return -1;
	}
	
	if ((unsigned)addr > USER_SIZE)
	{
		uerr(EFAULT);
		return -1;
	}
	
	for (i = 0; i < e; i++)
		if (!pt[i])
		{
			void *p = zpalloc();
			
			if (!p)
			{
				uerr(ENOMEM);
				return -1;
			}
			
			pt[i] = (unsigned)p | 7;
			curr->size++;
		}
	
	curr->brk = (unsigned)addr;
	return 0;
}

int sys_getegid(void)
{
	return curr->egid;
}

int sys_pause(void)
{
	while (!curr->sig)
		idle();
	uerr(EINTR);
	return -1;
}

int sys_setuid(uid_t uid)
{
	if (curr->euid && curr->ruid != uid)
	{
		uerr(EPERM);
		return -1;
	}
	curr->ruid = uid;
	curr->euid = uid;
	return 0;
}

int sys_setgid(gid_t gid)
{
	if (curr->euid && curr->rgid != gid)
	{
		uerr(EPERM);
		return -1;
	}
	curr->rgid = gid;
	curr->egid = gid;
	return 0;
}

pid_t sys_setsid(void)
{
	if (curr->psid == curr->pid)
	{
		uerr(EPERM);
		return -1;
	}
	curr->psid = curr->pid;
	inode_put(curr->tty);
	curr->tty = NULL;
	return 0;
}

int sys__iopl(void)
{
	if (curr->euid)
	{
		uerr(EPERM);
		return -1;
	}
	uregs->eflags |= 0x00003000;
	return 0;
}

unsigned sys__mfree(void)
{
	return pg_fcount << 12;
}

int sys_reboot(int mode)
{
	int write_super(struct super *sb);
	void hd_stop(void);
	void fd_stop(void);
	
	int i;
	
	for (i = 0; i < MAXINODES; i++)
		if (inode[i].refcnt)
		{
			inode[i].refcnt = 1;
			inode_put(&inode[i]);
		}
	init->root->sb->time = time.time;
	write_super(init->root->sb);
	sys_sync();
	fd_stop();
	
	if (mode)
	{
		printf("rebooting\n");
		asm volatile("lidt 0f; 0: .long 0,0");
		asm volatile("int $0");
	}
	
	hd_stop();
	printf("system halted\n");
	for (;;)
		asm volatile("hlt");
}

int sys__uptime(void)
{
	if (!boottime)
	{
		uerr(ENODEV);
		return -1;
	}
	return time.time - boottime;
}

void *sys_sbrk(ptrdiff_t incr)
{
	char *addr = (char *)curr->brk + incr;
	void *base = (void *)curr->brk;
	
	if (incr < 0 || incr >= USER_SIZE)
	{
		uerr(EINVAL);
		return (void *)-1;
	}
	
	if ((unsigned)addr > uregs->esp)
	{
		uerr(ENOMEM);
		return (void *)-1;
	}
	
	if (sys_brk(addr))
		return (void *)-1;
	
	return base;
}

int sys__newregion(void *base, size_t len)
{
	int err;
	
	err = newregion((unsigned)base, (unsigned)base + len);
	if (err)
	{
		uerr(err);
		return -1;
	}
	return 0;
}

int sys__setcompat(void *entry)
{
	curr->compat = (unsigned)entry;
	return 0;
}
