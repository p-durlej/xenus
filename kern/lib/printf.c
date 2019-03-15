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

#include <xenus/console.h>
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>

int con_putun(unsigned long v, unsigned int radix)
{
	int pz = 0;
	int i;
	
	for (i = 32; i >= 0; i--)
	{
		unsigned long digit = v;
		int c = i;
		char ch;
		
		while (c--)
			digit /= radix;
		digit %= (long)radix;
		
		if (!digit && !pz && i)
			continue;
		
		if (digit <= 9)
			ch = '0' + digit;
		else
			ch = 'a' + digit - 10;
		
		con_putk(ch);
		pz = 1;
	}
	
	return 0;
}

int con_putsn(long v, int radix)
{
	if (v < 0)
	{
		con_putk('-');
		v = -v;
	}
	
	return con_putun(v, radix);
}

int printf(char *format, ...)
{
	int *ap = (int *)(&format + 1);
	
	while (*format)
	{
		if (*format == '%')
			switch (*++format)
			{
			case '%':
				con_putk('%');
				break;
			case 's':
				con_puts((char *)*ap++);
				break;
			case 'c':
				con_putk(*ap++);
				break;
			case 'i':
				con_putsn(*ap++, 10);
				break;
			case 'u':
				con_putun(*ap++, 10);
				break;
			case 'o':
				con_putun(*ap++, 8);
				break;
			case 'x':
			case 'X':
				con_putun(*ap++, 16);
				break;
			case 'p':
				con_puts("0x");
				con_putun(*ap++, 16);
				break;
			default:
				con_putk('%');
				con_putk(*format);
			}
		else
			con_putk(*format);
		
		format++;
	}
	
	return 0;
}
