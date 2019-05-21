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

#include <xenus/process.h>
#include <xenus/syscall.h>
#include <xenus/config.h>
#include <xenus/clock.h>
#include <xenus/umem.h>
#include <xenus/intr.h>
#include <xenus/io.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

volatile struct systime time;
time_t boottime;

void fd_clock();

void clock_chkalarm()
{
	if (!curr->alarm.time)
		return;
	if (curr->alarm.time > time.time)
		return;
	if (curr->alarm.time == time.time && curr->alarm.ticks > time.ticks)
		return;
	curr->alarm.time = 0;
	sendsig(curr, SIGALRM);
}

void clkint(int nr, struct intr_regs *r)
{
	clock_chkalarm();
	
	if (r->cs & 3)
		curr->times.tms_utime++;
	else
		curr->times.tms_stime++;
	
	time.ticks++;
	if (time.ticks == HZ)
	{
		time.ticks = 0;
		time.time++;
		dosync = 1;
	}
	
	curr->time_slice--;
	if (curr->time_slice <= 0)
		resched = 1;
	
	fd_clock();
}

int clock_ticks(void)
{
	int r;
	
	intr_disable();
	r = (time.time - boottime) * HZ + time.ticks;
	intr_enable();
	return r;
}

void delay(int np)
{
	time_t t;
	int p;
	
	do
	{
		t = time.time;
		p = time.ticks;
	} while (time.time != t || time.ticks != p);
	
	p += np;
	t += p / HZ;
	p %= HZ;
	
	while (t > time.time);
	while (t == time.time && p > time.ticks);
}

void clock_init()
{
	outb(CLOCK_PORT + 3, 0x36);
	outb(CLOCK_PORT, (char)CLOCK_RELOADV);
	outb(CLOCK_PORT, CLOCK_RELOADV >> 8);
	
	irq_set(CLOCK_IRQ, clkint);
	irq_ena(CLOCK_IRQ);
}

time_t sys_time(time_t *t)
{
	time_t now = time.time;
	int err;
	
	if (t)
	{
		err = tucpy(t, &now, sizeof(now));
		if (err)
		{
			uerr(err);
			return -1;
		}
	}
	return now;
}

int sys_stime(time_t *t)
{
	time_t nt;
	int err;
	
	if (curr->euid)
	{
		uerr(EPERM);
		return -1;
	}
	
	err = fucpy(&nt, t, sizeof(nt));
	if (err)
	{
		uerr(err);
		return -1;
	}
	if (!boottime)
		boottime = nt;
	time.time = nt;
	return 0;
}

unsigned sys_alarm(unsigned sec)
{
	unsigned r = curr->alarm.time;
	struct systime at;
	time_t t;
	
	if (sec > 100000000)
	{
		uerr(EINVAL);
		return -1;
	}
	
	if (!sec)
	{
		curr->alarm.time = 0;
		t = time.time;
		if (at.time > t)
			return at.time - t;
		return 0;
	}
	
	do
	{
		at.time  = time.time;
		at.ticks = time.ticks;
	} while (at.time != time.time || at.ticks != time.ticks);
	
	if (r)
		r -= at.time;
	
	at.time += sec;
	intr_disable();
	curr->alarm = at;
	intr_enable();
	return r;
}

unsigned sys_sleep(unsigned sec)
{
	int t = clock_ticks() + sec * HZ;
	
	while (t > clock_ticks() && !curr->sig)
		idle();
	
	t = t - clock_ticks();
	if (t < 0)
		t = 0;
	
	return t / HZ;
}

clock_t sys_times(struct tms *tms)
{
	int err;
	
	if (tms)
	{
		err = tucpy(tms, &curr->times, sizeof curr->times);
		if (err)
		{
			uerr(err);
			return -1;
		}
	}
	return clock_ticks();
}

int sys__ftime(struct timeb *buf)
{
	struct timeb t;
	int err;
	
	memset(&t, 0, sizeof t);
	
	intr_disable();
	t.time	  = time.time;
	t.millitm = time.ticks * 1000 / HZ;
	intr_enable();
	
	err = tucpy(buf, &t, sizeof t);
	if (err)
	{
		uerr(err);
		return -1;
	}
	return 0;
}
