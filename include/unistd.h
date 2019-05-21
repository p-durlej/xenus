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

#ifndef _UNISTD_H
#define _UNISTD_H

#include <sys/types.h>
#include <sys/stat.h>

ssize_t	read(int fd, void *buf, size_t count);
ssize_t	write(int fd, void *buf, size_t count);
int	close(int fd);
int	unlink(char *name);
int	link(char *name1, char *name2);
off_t	lseek(int fd, off_t off, int whence);
int	access(char *name, int mode);
int	chown(char *path, uid_t uid, gid_t gid);
int	fchown(int fd, uid_t uid, gid_t gid);
int	pipe(int *pipedes);
int	dup(int fd);
int	dup2(int ofd, int nfd);
void	sync(void);

char *	getcwd(char *buf, int len);

int	chdir(char *path);
int	chroot(char *path);
int	rmdir(char *path);

unsigned alarm(unsigned sec);
unsigned sleep(unsigned sec);
int	pause(void);

pid_t	fork(void);
int	_exit(int status);

int	brk(void *addr);
void *	sbrk(ptrdiff_t incr);

pid_t	getpid(void);
pid_t	getppid(void);
pid_t	setsid(void);
pid_t	getpgrp(void);

uid_t	getuid(void);
uid_t	geteuid(void);
gid_t	getgid(void);
gid_t	getegid(void);
int	setuid(uid_t uid);
int	setgid(gid_t gid);

int	execve(char *path, char **argv, char **envp);
int	execv(char *path, char **argv);
int	execl(char *path, ...);
int	execle(char *path, ...);
int	execvp(char *path, char **argv);
int	execlp(char *path, ...);

int	ttyname3(int fd, char *buf, size_t len);
char *	ttyname(int fd);
int	isatty(int fd);

int	getpassn(char *buf, size_t len);

#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2

#define R_OK		4
#define W_OK		2
#define X_OK		1
#define F_OK		0

#define STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2

#endif
