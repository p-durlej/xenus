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

#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

struct format
{
	int alt;
	int sign;
	int zero;
	int left;
	int spc;
	
	int signedval;
	int width;
	int base;
};

void __libc_itoa(unsigned long v, int base, char *buf)
{
	int z = 0;
	int i;
	
	for (i = 32; i >= 0; i--)
	{
		unsigned long digit = v;
		char ch;
		int c = i;
		
		while (c--)
			digit /= base;
		digit %= base;
		
		if (!digit && !z && i)
			continue;
		
		if (digit <= 9)
			ch = '0' + digit;
		else
			ch = 'a' + digit - 10;
		
		*buf = ch;
		buf++;
		z = 1;
	}
	*buf = 0;
}

int __libc_fmtputs(FILE *f, struct format *fmt, char *str)
{
	int l = strlen(str);
	char spc = ' ';
	int nspc = 0;
	
	if (fmt->zero)
		spc = '0';
	
	if (l < fmt->width)
		nspc = fmt->width - l;
	
	if (!fmt->left)
		while(nspc--)
			if (fputc(spc, f) == EOF)
				return -1;
	fputs(str, f);
	if (fmt->left)
		while(nspc--)
			if (fputc(spc, f) == EOF)
				return -1;
	return 0;
}

int __libc_fmtputn(FILE *f, struct format *fmt, va_list *ap)
{
	unsigned long v;
	char buf[34];
	char *p = buf;
	int neg = 0;
	
	if (fmt->signedval)
	{
		int arg = va_arg(*ap, int);
		
		if (arg < 0)
		{
			v   = -arg;
			neg = 1;
		}
		else
			v = arg;
	}
	else
		v = va_arg(*ap, unsigned);
	
	if (neg)
	{
		*p = '-';
		p++;
	}
	else
	{
		if (fmt->sign)
		{
			*p = '+';
			p++;
		}
		else
		if (fmt->spc)
		{
			*p = ' ';
			p++;
		}
	}
	
	__libc_itoa(v, fmt->base, p);
	return __libc_fmtputs(f, fmt, buf);
}

int vfprintf(FILE *f, char *format, va_list ap)
{
	struct format fmt;
	char ch;
	
	while (*format)
	{
		if (*format == '%')
		{
			format++;
			
			if (*format == '%')
			{
				if (fputc('%', f) == EOF)
					return -1;
				format++;
				continue;
			}
			
			fmt.spc		= 0;
			fmt.alt		= 0;
			fmt.sign	= 0;
			fmt.zero	= 0;
			fmt.width	= 0;
			fmt.left	= 0;
			fmt.signedval	= 1;
			
			while (ch = *format, ch)
			{
				format++;
				
				switch (ch)
				{
				case '0':
					fmt.zero = 1;
					break;
				case '-':
					fmt.left = 1;
					break;
				case '#':
					fmt.alt = 1;
					break;
				case ' ':
					fmt.sign = 0;
					fmt.spc	 = 1;
					break;
				case '+':
					fmt.sign = 1;
					fmt.spc	 = 0;
					break;
				default:
					format--;
					goto getwidth;
				}
			}
			
getwidth:			
			while (isdigit(*format))
			{
				fmt.width *= 10;
				fmt.width += *format-'0';
				format++;
			}
			
			while (ch = *format, ch)
			{
				format++;
				
				switch (ch)
				{
				case 'l':
					break;
				default:
					format--;
					goto convert;
				}
			}
			
convert:
			switch (*format)
			{
			case 'm':
				if (__libc_fmtputs(f, &fmt, strerror(errno)))
					return -1;
				break;
			case 's':
				if (__libc_fmtputs(f, &fmt, va_arg(ap, char *)))
					return -1;
				break;
			case 'c':
				if (fputc(va_arg(ap, char), f) == EOF)
					return -1;
				break;
			case 'd':
			case 'i':
				fmt.base	= 10;
				fmt.signedval	= 1;
				if (__libc_fmtputn(f, &fmt, &ap))
					return -1;
				break;
			case 'u':
				fmt.base	= 10;
				fmt.signedval	= 0;
				if (__libc_fmtputn(f, &fmt, &ap))
					return -1;
				break;
			case 'o':
				fmt.base	= 8;
				fmt.signedval	= 0;
				if (__libc_fmtputn(f, &fmt, &ap))
					return -1;
				break;
			case 'x':
			case 'X':
			case 'p':
				fmt.base	= 16;
				fmt.signedval	= 0;
				if (__libc_fmtputn(f, &fmt, &ap))
					return -1;
				break;
			default:
				if (fputc('%', f) == EOF)
					return -1;
				if (fputc(*format, f) == EOF)
					return -1;
			}
		}
		else if (fputc(*format, f) == EOF)
			return -1;
		
		format++;
	}
	return 0;
}
