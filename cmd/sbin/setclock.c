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

#include <xenus/io.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

#define IOBASE 0x70

int _iopl();

unsigned intbcd(unsigned v)
{
	return (v % 10) + ((v / 10) << 4);
}

static void rtc_write(void)
{
	struct tm *tm;
	time_t t;
	
	time(&t);
	tm = localtime(&t);
	
	outb(IOBASE,	 0x00);
	outb(IOBASE + 1, 0x00);
	
	outb(IOBASE,	 0x09);
	outb(IOBASE + 1, intbcd(tm->tm_year - 100));
	
	outb(IOBASE,	 0x08);
	outb(IOBASE + 1, intbcd(tm->tm_mon + 1));
	
	outb(IOBASE,	 0x07);
	outb(IOBASE + 1, intbcd(tm->tm_mday));
	
	outb(IOBASE,	 0x06);
	outb(IOBASE + 1, intbcd(tm->tm_wday + 1));
	
	outb(IOBASE,	 0x04);
	outb(IOBASE + 1, intbcd(tm->tm_hour));
	
	outb(IOBASE,	 0x02);
	outb(IOBASE + 1, intbcd(tm->tm_min));
	
	outb(IOBASE,	 0x00);
	outb(IOBASE + 1, intbcd(tm->tm_sec));
}

int main(int argc, char **argv)
{
	if (_iopl())
	{
		perror(NULL);
		return 1;
	}
	
	rtc_write();
	return 0;
}
