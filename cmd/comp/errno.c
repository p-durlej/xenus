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

#include <errno.h>

#include "mnxcompat.h"

static int errtab[] =
{
	[ENOENT]	=  2,
	[EACCES]	= 13,
	[EPERM]		=  1,
	[E2BIG]		=  7,
	[EBUSY]		= 16,
	[ECHILD]	= 10,
	[EEXIST]	= 17,
	[EFAULT]	= 14,
	[EFBIG]		= 27,
	[EINTR]		=  4,
	[EINVAL]	= 22,
	[ENOMEM]	= 12,
	[EMFILE]	= 24,
	[EMLINK]	= 31,
	[EAGAIN]	= 11,
	[ENAMETOOLONG]	= 36,
	[ENFILE]	= 23,
	[ENODEV]	= 19,
	[ENOEXEC]	=  8,
	[EIO]		=  5,
	[ENOSPC]	= 28,
	[ENOSYS]	= 38,
	[ENOTDIR]	= 20,
	[ENOTEMPTY]	= 39,
	[ENOTTY]	= 25,
	[EPIPE]		= 32,
	[EROFS]		= 30,
	[ESPIPE]	= 29,
	[ESRCH]		=  3,
	[EXDEV]		= 18,
	[ERANGE]	= 34,
	[EBADF]		=  9,
	[EISDIR]	= 21,
	[ETXTBSY]	= 26,
	[ENXIO]		=  6,
};

int mnx_error(int err)
{
#if MNXDEBUG
//	mprintf("mnx_error %i -> %i\n", err, errtab[err]);
#endif
	return -errtab[err];
}
