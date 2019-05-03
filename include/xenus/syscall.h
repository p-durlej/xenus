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

#include <xenus/intr.h>
#include <xenus/fs.h>
#include <sys/utsname.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <utime.h>

extern struct intr_regs *uregs;

void uerr(int errno);

int	sys__sysmesg(char *msg, unsigned int len);
int	sys__exit(int status);
int	sys_brk(void *addr);
int	sys_getpid(void);
int	sys_getppid(void);
int	sys_getuid(void);
int	sys_geteuid(void);
int	sys_getgid(void);
int	sys_getegid(void);
void *	sys_signal(int signum, void *handler);
int	sys_kill(pid_t pid, int sig);
int	sys_mknod(char *path, mode_t mode, dev_t dev);
int	sys_open(char *path, int flags, mode_t mode);
int	sys_close(int fd);
ssize_t	sys_read(int fd, void *buf, size_t count);
ssize_t	sys_write(int fd,void *buf, size_t count);
off_t	sys_lseek(int fd, off_t off, int whence);
int	sys_fstat(int fd, struct stat *buf);
int	sys_fchmod(int fd, mode_t mode);
int	sys_fchown(int fd, uid_t uid, gid_t gid);
int	sys_access(char *name, int mode);
int	sys_chdir(char *path);
int	sys_chroot(char *path);
int	sys_unlink(char *name);
int	sys_mkdir(char *path, mode_t mode);
int	sys_rmdir(char *path);
int	sys_link(char *name1, char *name2);
int	sys_rename(char *name1, char *name2);
int	sys_ioctl(int fd, int cmd, void *ptr);
int	sys_fcntl(int fd, int cmd, long arg);
int	sys__exec(char *name, char *arg, char *env);
pid_t	sys_fork(void);
pid_t	sys_wait(int *status);
int	sys_pause(void);
time_t	sys_time(time_t *t);
int	sys_stime(time_t *t);
unsigned sys_alarm(unsigned sec);
int	sys_setuid(uid_t uid);
int	sys_setgid(gid_t gid);
pid_t	sys_setsid(void);
int	sys_ttyname3(int fd, char *buf, size_t len);
int	sys_statfs(char *path, struct statfs *buf);
int	sys_mount(char *dev, char *mnt, int ro);
int	sys_umount(char *mnt);
int	sys_sync(void);
int	sys_umask(mode_t mask);
int	sys__ctty(char *path);
int	sys_pipe(int *buf);
int	sys__killf(char *path, int sig);
int	sys_uname(struct utsname *buf);
int	sys__iopl(void);
unsigned sys__mfree(void);
int	sys_reboot(int mode);
int	sys_stat(char *path, struct stat *buf);
int	sys_chmod(char *path, mode_t mode);
int	sys_chown(char *path, uid_t uid, gid_t gid);
int	sys_dup(int fd);
int	sys_dup2(int ofd, int nfd);
int	sys__uptime(void);
void *	sys_sbrk(ptrdiff_t incr);
int	sys__dmesg(char *buf, int len);
unsigned sys_sleep(unsigned sec);
int	sys__killu(uid_t ruid, int sig);
int	sys_utime(char *path, struct utimbuf *tp);
int	sys__newregion(void *base, size_t len);
int	sys__setcompat(void *entry);
pid_t	sys_waitpid(pid_t pid, int *status, int options);
clock_t	sys_times(struct tms *tms);
