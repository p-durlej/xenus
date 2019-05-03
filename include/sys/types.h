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

#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H

typedef char		i8_t;
typedef short		i16_t;
typedef long		i32_t;

typedef unsigned char	u8_t;
typedef unsigned short	u16_t;
typedef unsigned int	u32_t;

typedef i32_t	sig_t;
typedef i32_t	pid_t;
typedef i16_t	uid_t;
typedef uid_t	gid_t;
typedef u32_t	mode_t;
typedef u32_t	dev_t;
typedef u32_t	size_t;
typedef i32_t	ssize_t;
typedef i32_t	ptrdiff_t;
typedef i32_t	off_t;
typedef u32_t	ino_t;
typedef i32_t	time_t;
typedef u32_t	nlink_t;
typedef u32_t	blk_t;
typedef blk_t	blkcnt_t;
typedef u32_t	blksize_t;

typedef unsigned clock_t;

#endif
