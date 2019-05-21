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

#include <sys/ftime.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>

#define isleap(y)	(!(y % 4) && ((y % 100) || (y % 400)))
#define yearsecs(y)	((365 + isleap(y)) * 86400)
#define monthdays(y,m)	((isleap(y) && m==1) ? __libc_monthlen[m]+1 : \
                         __libc_monthlen[m])
#define monthsecs(y,m)	(monthdays(y,m) * 86400)

char *__libc_wday[7]=
{
	"Sun",
	"Mon",
	"Tue",
	"Wed",
	"Thu",
	"Fri",
	"Sat"
};

char *__libc_month[12]=
{
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec"
};

int __libc_monthlen[12]={ 31,28,31,30,31,30,31,31,30,31,30,31 };

char *asctime(struct tm *time)
{
	static char buf[64];
	
	sprintf(buf,"%s %s %2i %02i:%02i:%02i %i\n",
	        __libc_wday[time->tm_wday],
		__libc_month[time->tm_mon],
		time->tm_mday,
		time->tm_hour,
		time->tm_min,
		time->tm_sec,
		time->tm_year + 1900);
	
	return buf;
}

char *ctime(time_t *time)
{
	return asctime(localtime(time));
}

struct tm *gmtime(time_t *time)
{
	static struct tm tm;
	time_t t = *time;
	int i = 0;
	
	tm.tm_wday = (t / 86400 + 4) % 7;
	tm.tm_sec  = t % 60;
	tm.tm_min  = (t / 60) % 60;
	tm.tm_hour = (t / 3600) % 24;
	tm.tm_year = 70;
	tm.tm_mon = 0;
	while (t >= yearsecs(tm.tm_year))
	{
		t -= yearsecs(tm.tm_year);
		tm.tm_year++;
	}
	tm.tm_yday = t / 86400;
	while (t >= monthsecs(tm.tm_year, tm.tm_mon))
	{
		t -= monthsecs(tm.tm_year, tm.tm_mon);
		tm.tm_mon++;
	}
	tm.tm_mday  = t / 86400 + 1;
	tm.tm_isdst = -1;
	return &tm;
}

struct tm *localtime(time_t *time)
{
	time_t t;
	
	tzset();
	t = timezone + *time;
	return gmtime(&t);
}

time_t timegm(struct tm *time)
{
	struct tm tm;
	time_t t;
	
	tm = *time;
	t  = (tm.tm_mday-1) * 86400;
	t +=  tm.tm_hour    * 3600;
	t +=  tm.tm_min	    * 60;
	t +=  tm.tm_sec;
	while (tm.tm_mon)
	{
		tm.tm_mon--;
		t += monthsecs(tm.tm_year, tm.tm_mon);
	}
	while (tm.tm_year > 70)
	{
		tm.tm_year--;
		t += yearsecs(tm.tm_year);
	}
	return t;
}

time_t mktime(struct tm *time)
{
	tzset();
	return timegm(time) - timezone;
}
