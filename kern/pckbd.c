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
#include <xenus/config.h>
#include <xenus/ttyld.h>
#include <xenus/intr.h>
#include <xenus/io.h>
#include <sys/types.h>

#define SHIFT		1
#define CTRL		2
#define ALT		4

#define CTRL_SCAN	0x1d
#define LSHIFT_SCAN	0x2a
#define RSHIFT_SCAN	0x36
#define ALT_SCAN	0x38
#define CAPS_SCAN	0x3a
#define NLOCK_SCAN	0x45
#define SLOCK_SCAN	0x46
#define DEL_SCAN	0x53

extern struct tty con_ttyld;

unsigned pckbd_buf[PCKBD_BUFSIZE];
unsigned pckbd_shift;
unsigned pckbd_outp;
unsigned pckbd_inp;

char pckbd_dfltmap[] =
{
	0,0x1b,'1','2','3','4','5','6',
	'7','8','9','0','-','=','\b','\t',
	'q','w','e','r','t','y','u','i',
	'o','p','[',']','\n',0,'a','s',
	'd','f','g','h','j','k','l',';',
	'\'','`',0,'\\','z','x','c','v',
	'b','n','m',',','.','/',0,'*',
	0,' ',0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	'k',0,'-','h','5','l','+',0,
	'j',0,0,127,0,0,'\\',0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
};

char pckbd_shiftmap[] =
{
	0,0x1b,'!','@','#','$','%','^',
	'&','*','(',')','_','+','\b','\t',
	'Q','W','E','R','T','Y','U','I',
	'O','P','{','}','\n',0,'A','S',
	'D','F','G','H','J','K','L',':',
	'"','~',0,'|','Z','X','C','V',
	'B','N','M','<','>','?',0,'*',
	0,' ',0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	'k',0,'-','h','5','l','+',0,
	'j',0,0,127,0,0,'|',0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
};

char pckbd_altmap[]=
{
	0,0x1b,'1','2','3','4','5','6',
	'7','8','9','0','-','=','\b','\t',
	'q','w','e','r','t','y','u','i',
	'o','p','[',']','\n',0,'a','s',
	'd','f','g','h','j','k','l',';',
	'\'','`',0,'\\','z','x','c','v',
	'b','n','m',',','.','/',0,'*',
	0,' ',0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	'k',0,'-','h','5','l','+',0,
	'j',0,0,127,0,0,'\\',0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
};

void cdebug(void)
{
#if CDEBUGBLK
	cdebugblk();
#endif
#if CDEBUGRS
	cdebugrs();
#endif
#if CDEBUGPROC
	cdebugproc();
#endif
}

int con_kbhit()
{
	if (pckbd_inp == pckbd_outp)
		return 0;
	return pckbd_buf[pckbd_outp];
}

int con_getch()
{
	char ch;
	
	while (!(ch = con_kbhit()));
	pckbd_outp++;
	pckbd_outp %= PCKBD_BUFSIZE;
	
	return ch;
}

void pckbd_put(char ch)
{
	if (ttyld_intr(&con_ttyld, ch))
		return;
	
	pckbd_buf[pckbd_inp] = ch;
	pckbd_inp++;
	pckbd_inp %= PCKBD_BUFSIZE;
	if (pckbd_inp == pckbd_outp)
		con_putc('\a');
}

void pckbd_reset()
{
	intr_disable();
	outb(PCKBD_PORT + 4, 0xfe);
	for (;;);
}

void pckbd_intr()
{
	unsigned code = inb(PCKBD_PORT);
	char c;
	
	if (code == DEL_SCAN && (pckbd_shift & CTRL) && (pckbd_shift & ALT))
		pckbd_reset();
	
	switch (code)
	{
	case CTRL_SCAN:
	case CAPS_SCAN:
		pckbd_shift |= CTRL;
		break;
	case LSHIFT_SCAN:
	case RSHIFT_SCAN:
		pckbd_shift |= SHIFT;
		break;
	case ALT_SCAN:
		pckbd_shift |= ALT;
		break;
	}
	
	if (code & 0x80)
		switch (code & 0x7f)
		{
		case CTRL_SCAN:
		case CAPS_SCAN:
			pckbd_shift &= ~CTRL;
			break;
		case LSHIFT_SCAN:
		case RSHIFT_SCAN:
			pckbd_shift &= ~SHIFT;
			break;
		case ALT_SCAN:
			pckbd_shift &= ~ALT;
			break;
		}
	
	if (code & 0x80)
		return;
	
	c = pckbd_dfltmap[code];
	if (pckbd_shift & SHIFT)
		c = pckbd_shiftmap[code];
	if (pckbd_shift & ALT)
		c = pckbd_altmap[code];
	if (pckbd_shift & CTRL)
		c &= 31;
	
	if (c == 'd' && (pckbd_shift == ALT))
	{
		cdebug();
		return;
	}
	if (c)
		pckbd_put(c);
}

void pckbd_init()
{
	inb(PCKBD_PORT);
	
	irq_set(PCKBD_IRQ, pckbd_intr);
	irq_ena(PCKBD_IRQ);
}
