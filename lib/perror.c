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
#include <string.h>
#include <stdio.h>
#include <errno.h>

char *sys_errlist[] =
{
	"Success",			// 0
	"No such file or directory",	// 1
	"Permission denied",		// 2
	"Operation not permitted",	// 3
	"Argument list too big",	// 4
	"Device busy",			// 5
	"No child processes",		// 6
	"File exists",			// 7
	"Invalid address",		// 8
	"File too big",			// 9
	"System call interrupted",	// 10
	"Invalid value",		// 11
	"Not enough memory",		// 12
	"Too many open files",		// 13
	"Too many links",		// 14
	"Try again",			// 15
	"File name too long",		// 16
	"Too many open files in system",// 17
	"No such device (ENODEV)",	// 18
	"Not an executable file",	// 19
	"I/O error",			// 20
	"No space left on device",	// 21
	"System call not implemented",	// 22
	"Not a directory",		// 23
	"Directory not empty",		// 24
	"Not a tty",			// 25
	"Broken pipe",			// 26
	"Read-only file system",	// 27
	"Illegal seek",			// 28
	"No such process",		// 29
	"Cross-device link",		// 30
	"Result too large",		// 31
	"Bad file descriptor",		// 32
	"Is a directory",		// 33
	"Text file busy",		// 34
	"No such device (ENXIO)",	// 35
};

char *strerror(int errno)
{
	static char msg[32];
	
	if (errno < 0 || errno >= sizeof sys_errlist / sizeof *sys_errlist)
	{
		sprintf(msg, "Unknown error %i", errno);
		return msg;
	}
	return sys_errlist[errno];
}

void perror(char *str)
{
	char *msg = strerror(errno);
	
	if (str && *str)
	{
		write(2, str, strlen(str));
		write(2, ": ", 2);
	}
	write(2, msg, strlen(msg));
	write(2, "\n", 1);
}
