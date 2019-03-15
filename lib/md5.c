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

#include <string.h>
#include <stdio.h>

typedef unsigned uint32_t;

static uint32_t s[64] =
{
	7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
	5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
	4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
	6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21,
};

static uint32_t K[64] =
{
	0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
	0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
	0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
	0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
	0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
	0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
	0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
	0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
	0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
	0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
	0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
	0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
	0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
	0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
	0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
	0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
};

static uint32_t leftrotate(uint32_t x, int c)
{
	return (x << c) | (x >> (32 - c));
}

char *md5(char *password)
{
	static uint32_t digest[4];
	
	digest[0] = 0x67452301;
	digest[1] = 0xefcdab89;
	digest[2] = 0x98badcfe;
	digest[3] = 0x10325476;
	
	uint32_t A = digest[0];
	uint32_t B = digest[1];
	uint32_t C = digest[2];
	uint32_t D = digest[3];
	
	uint32_t msg[16];
	int len;
	int i;
	
	len = strlen(password);
	len = len > 8 ? 8 : len;
	
	memset(msg, 0, sizeof msg);
	memcpy(msg, password, len);
	((unsigned char *)msg)[len] = 128;
	msg[14] = len;
	
	for (i = 0; i < 64; i++)
	{
		uint32_t F, g;
		
		if (i < 16)
		{
			F = (B & C) | (~B & D);
			g = i;
		}
		else if (i < 32)
		{
			F = (D & B) | (~D & C);
			g = (5 * i + 1) % 16;
		}
		else if (i < 48)
		{
			F = B ^ C ^ D;
			g = (3 * i + 5) % 16;
		}
		else
		{
			F = C ^ (B | ~D);
			g = (7 * i) % 16;
		}
		F += A + K[i] + msg[g];
		A = D;
		D = C;
		C = B;
		B = B + leftrotate(F, s[i]);
	}
	
	digest[0] += A;
	digest[1] += B;
	digest[2] += C;
	digest[3] += D;
	
	return (char *)digest;
}

char *md5a(char *password)
{
	static char cs[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+$";
	static char buf[33];
	
	unsigned char *p = (unsigned char *)md5(password);
	int i;
	
	for (i = 0; i < 16; i++)
		buf[i] = cs[p[i] & 63];
//		sprintf(buf + i * 2, "%02x", p[i]);
	return buf;
}
