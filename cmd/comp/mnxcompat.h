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

#define STXT 0x200000

#define MNXDEBUG 0

#ifndef __ASSEMBLER__

struct message
{
	int source;
	int type;
	union
	{
		struct { int i1, i2, i3; void *p1, *p2, *p3;		} m1;
		struct { int i1, i2, i3; long l1, l2; void *p1;		} m2;
		struct { int i1, i2; void *p1; char str[14];		} m3;
		struct { long l1, l2, l3, l4, l5;			} m4;
		struct { char c1, c2; int i1, i2; long l1, l2, l3;	} m5;
		struct { int i1, i2, i3; long l1; void *sigh;		} m6;
	};
};

extern int sig_m2x[];
extern int sig_x2m[];

int mnx_read(struct message *msg);
int mnx_write(struct message *msg);
int mnx_open(struct message *msg);
int mnx_close(struct message *msg);
int mnx_lseek(struct message *msg);
int mnx_ioctl(struct message *msg);
int mnx_mkdir(struct message *msg);
int mnx_rmdir(struct message *msg);
int mnx_unlink(struct message *msg);
int mnx_stat(struct message *msg);
int mnx_fstat(struct message *msg);
int mnx_access(struct message *msg);
int mnx_fcntl(struct message *msg);
int mnx_chdir(struct message *msg);
int mnx_creat(struct message *msg);
int mnx_link(struct message *msg);
int mnx_mknod(struct message *msg);
int mnx_chmod(struct message *msg);
int mnx_chown(struct message *msg);
int mnx_dup(struct message *msg);
int mnx_pipe(struct message *msg);
int mnx_chroot(struct message *msg);
int mnx_utime(struct message *msg);
int mnx_mount(struct message *msg);
int mnx_umount(struct message *msg);

int mnx_sigaction(struct message *msg);
int mnx_sigsuspend(struct message *msg);
int mnx_sigpending(struct message *msg);
int mnx_sigprocmask(struct message *msg);
int mnx_signal(struct message *msg);

extern unsigned entry;
extern unsigned ibrk;
extern unsigned cs;

extern int trace;

int mprintf(char *format, ...);
void panic(char *msg);
void load(char *path, int pfd);
void start(char *arg, char *env);
void siginit(void);

void callsig(void *proc, int nr);
void enter(void *stack);

int mnx_error(int err);

#endif
