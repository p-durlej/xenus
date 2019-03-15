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

#include <unistd.h>
#include <limits.h>
#include <stdio.h>

FILE __libc_file[OPEN_MAX];

FILE *stdin  = &__libc_file[0];
FILE *stdout = &__libc_file[1];
FILE *stderr = &__libc_file[2];

void __libc_stdio_init(void)
{
	int se;
	int i;
	
	for (i = 0; i < OPEN_MAX; i++)
		__libc_file[i].fd = -1;
	
	stdin->fd	  = STDIN_FILENO;
	stdin->mode	  = __LIBC_FMODE_R;
	stdin->buf_mode	  = _IOFBF;
	stdin->ungotc	  = EOF;
	
	stdout->fd	  = STDOUT_FILENO;
	stdout->mode	  = __LIBC_FMODE_W;
	stdout->buf_mode  = _IOLBF;
	stdout->ungotc	  = EOF;
	
	stderr->fd	  = STDERR_FILENO;
	stderr->mode	  = __LIBC_FMODE_W;
	stderr->buf_mode  = _IONBF;
	stderr->ungotc	  = EOF;
}

void __libc_stdio_cleanup()
{
	int i;
	
	for (i = 0; i < OPEN_MAX; i++)
		if (__libc_file[i].fd != -1 && __libc_file[i].buf_write)
			fflush(&__libc_file[i]);
}
