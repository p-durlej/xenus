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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

typedef long INT;

#define T_INT	1
#define T_STR	2

#define P_MOD	0
#define P_DIV	1
#define P_MUL	2
#define P_SUB	3
#define P_ADD	3
#define P_CMP	4
#define P_AND	5
#define P_OR	6
#define P_ALL	7

struct value
{
	int type;
	union
	{
		INT integer;
		char *string;
	};
};

char **token;

void getsingleval(struct value *buf);
void getvalue(struct value *buf, int p);
int main(int argc, char **argv);

void getsingleval(struct value *buf)
{
	char *p;
	INT iv;
	
	if (!*token)
	{
		fputs("Unexpected end of expression\n", stderr);
		exit(1);
	}
	
	if (!strcmp(*token, "("))
	{
		token++;
		getvalue(buf, P_ALL);
		if (!*token || strcmp(*token, ")"))
		{
			fputs("'(' with no corresponding ')'\n", stderr);
			exit(1);
		}
		token++;
		return;
	}
	
	for (p = *token; *p; p++)
		if (!isdigit(*p))
		{
			buf->type   = T_STR;
			buf->string = strdup(*token);
			if (!buf->string)
			{
				perror(*token);
				exit(1);
			}
			token++;
			return;
		}
	
	buf->integer = atoi(*token);
	buf->type    = T_INT;
	token++;
}

void badtype(void)
{
	fputs("Type mismatch\n", stderr);
	exit(1);
}

void getvalue(struct value *buf, int p)
{
	struct value v1;
	struct value v2;
	
	getsingleval(&v1);
	
	while (*token)
	{
		if (p < P_MOD)
			break;
		if (!strcmp(*token, "%"))
		{
			token++;
			getvalue(&v2, P_MOD);
			if (v1.type != T_INT || v2.type != T_INT)
				badtype();
			v1.integer %= v2.integer;
			continue;
		}
		if (p < P_DIV)
			break;
		if (!strcmp(*token, "/"))
		{
			token++;
			getvalue(&v2, P_DIV);
			if (v1.type != T_INT || v2.type != T_INT)
				badtype();
			v1.integer /= v2.integer;
			continue;
		}
		if (p < P_MUL)
			break;
		if (!strcmp(*token, "*"))
		{
			token++;
			getvalue(&v2, P_MUL);
			if (v1.type != T_INT || v2.type != T_INT)
				badtype();
			v1.integer *= v2.integer;
			continue;
		}
		if (p < P_ADD)
			break;
		if (!strcmp(*token, "+"))
		{
			token++;
			getvalue(&v2, P_ADD);
			if (v1.type != T_INT || v2.type != T_INT)
				badtype();
			v1.integer += v2.integer;
			continue;
		}
		if (!strcmp(*token, "-"))
		{
			token++;
			getvalue(&v2, P_SUB);
			if (v1.type != T_INT || v2.type != T_INT)
				badtype();
			v1.integer -= v2.integer;
			continue;
		}
		if (p < P_CMP)
			break;
		if (!strcmp(*token, "<"))
		{
			token++;
			getvalue(&v2, P_CMP);
			if (v1.type != T_INT || v2.type != T_INT)
				badtype();
			v1.integer = v1.integer < v2.integer;
			continue;
		}
		if (!strcmp(*token, "<="))
		{
			token++;
			getvalue(&v2, P_CMP);
			if (v1.type != T_INT || v2.type != T_INT)
				badtype();
			v1.integer = v1.integer <= v2.integer;
			continue;
		}
		if (!strcmp(*token, ">"))
		{
			token++;
			getvalue(&v2, P_CMP);
			if (v1.type != T_INT || v2.type != T_INT)
				badtype();
			v1.integer = v1.integer > v2.integer;
			continue;
		}
		if (!strcmp(*token, ">="))
		{
			token++;
			getvalue(&v2, P_CMP);
				badtype();
			v1.integer = v1.integer >= v2.integer;
			continue;
		}
		if (!strcmp(*token, "=="))
		{
			token++;
			getvalue(&v2, P_CMP);
			if (v1.type != T_INT || v2.type != T_INT)
				badtype();
			v1.integer = v1.integer == v2.integer;
			continue;
		}
		if (!strcmp(*token, "!="))
		{
			token++;
			getvalue(&v2, P_CMP);
			if (v1.type != T_INT || v2.type != T_INT)
				badtype();
			v1.integer = v1.integer != v2.integer;
			continue;
		}
		if (p < P_AND)
			break;
		if (!strcmp(*token, "&"))
		{
			token++;
			getvalue(&v2, P_CMP);
			
			if (v2.type == T_INT && !v2.integer)
			{
				v1.type	   = T_INT;
				v1.integer = 0;
			}
			if (v2.type == T_STR && !*v2.string)
			{
				v1.type	   = T_INT;
				v1.integer = 0;
			}
			if (v1.type == T_STR && !*v1.string)
			{
				v1.type	   = T_INT;
				v1.integer = 0;
			}
			
			continue;
		}
		if (p < P_OR)
			break;
		if (!strcmp(*token, "|"))
		{
			token++;
			getvalue(&v2, P_CMP);
			
			if (v1.type == T_INT && !v1.integer)
				v1 = v2;
			if (v1.type == T_STR && !*v1.string)
				v1 = v2;
			
			continue;
		}
		break;
	}
	*buf = v1;
}

int main(int argc,char **argv)
{
	struct value v;
	
	if (argc < 2)
	{
		printf("\nUsage: expr EXPR\n"
			"Evaluate EXPR and print the result.\n\n"
			" Supported operators are:\n\n"
			"  %%                        (priority 0)\n"
			"  /                        (priority 1)\n"
			"  *                        (priority 2)\n"
			"  +, -                     (priority 3)\n"
			"  <, <=, >, >=, ==, !=     (priority 4)\n"
			"  &                        (priority 5)\n"
			"  |                        (priority 6)\n\n"
			" The lower number the higher priority.\n\n"
			);
		return 1;
	}
	
	token = argv + 1;
	getvalue(&v, P_ALL);
	
	if (*token)
	{
		fputs("Junk after expression\n", stderr);
		return 1;
	}
	
	switch (v.type)
	{
	case T_INT:
		printf("%li\n", v.integer);
		return 0;
	case T_STR:
		puts(v.string);
		return 0;
	}
}
