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

#define V7DEBUG 0

#ifndef __ASSEMBLER__

extern unsigned entry;
extern unsigned ibrk;

extern char *ubin;

int mprintf(char *format, ...);
void panic(char *msg);
void load(char *path, int pfd);
void start(char *arg, char *env);

void enter(void *stack);

char *xbin(char *path);

int v7_ioctl(int fd, int cmd, void *data);
int v7_read(int fd, void *buf, int len);
int v7_stat(char *path, void *st7);
int v7_fstat(int fd, void *st7);
int v7_open(char *path, int mode);
int v7_dup(int ofd, int nfd);
int v7_chmod(char *path, unsigned mode);
int v7_creat(char *path, unsigned mode);
int v7_mknod(char *path, unsigned mode, unsigned rdev);
int v7_mkfifo(char *path, unsigned mode);
int v7_mkdir(char *path, unsigned mode);
int v7_chdir(char *path);
int v7_chroot(char *path);
int v7_unlink(char *name);
int v7_link(char *name1, char *name2);
int v7_access(char *name, int mode);
int v7_chown(char *path, int uid, int gid);
int v7_mount(char *dev, char *mnt, int ro);
int v7_umount(char *mnt);

int v7_error(int err);

#endif
