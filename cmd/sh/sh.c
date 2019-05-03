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

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>

#define SHELL
#include "glob.c"

#define GLOB_CHRS	"*?"

#define ERR_SYNTAX	127
#define ERR_EXEC	127
#define ERR_FORK	127
#define ERR_MEM		127
#define ERR_GETCWD	127
#define ERR_CHDIR	127
#define ERR_ARGC	127
#define ERR_ARGS	127
#define ERR_OPEN	127
#define ERR_STAT	127
#define ERR_SEEK	127
#define ERR_NOLABEL	1
#define ERR_UNK		127

#define MAXENV		64

extern char **environ;

char *nenv[MAXENV + 1];
char envdup[MAXENV];

typedef int cmd_proc(int argc, char **argv);

static int cmd_noop(int argc, char **argv);
static int cmd_exit(int argc, char **argv);
static int cmd_chdir(int argc, char **argv);
static int cmd_goto(int argc, char **argv);
static int cmd_umask(int argc, char **argv);
static int cmd_newgrp(int argc, char **argv);
static int cmd_unset(int argc, char **argv);
static int cmd_exec(int argc, char **argv);

static int waitexit(int st);
static char *readln(void);
static int otoi(char *nptr);

static struct comm
{
	char *		name;
	cmd_proc *	proc;
} comms[] =
{
	{ ":",		cmd_noop	},
	{ "exit",	cmd_exit	},
	{ "chdir",	cmd_chdir	},
	{ "cd",		cmd_chdir	},
	{ "goto",	cmd_goto	},
	{ "umask",	cmd_umask	},
	{ "newgrp",	cmd_newgrp	},
	{ "unset",	cmd_unset	},
	{ "exec",	cmd_exec	},
};

extern char *	__progname;

static char	sbuf[4096];
static char *	args[100];
static char	lastprompt[256];
static char *	promptp;
static int	logsh;
static void *	sigquit;
static void *	sigterm;
static void *	sigint;
static int	lastst;
static int	stop;
static char *	source_name;
static int	source_nxit;
static FILE *	source;
static char **	sargv;
static int	sargc;
static int	varc;
static char *	arg0;

static int cmd_noop(int argc, char **argv)
{
	return 0;
}

static int cmd_exit(int argc, char **argv)
{
	int x = lastst;
	
	if (argc > 1)
		x = atoi(argv[1]);
	exit(x);
	return 0; /* XXX silence a clang warning */
}

static int cmd_chdir(int argc, char **argv)
{
	char *d;
	
	if (argc < 2)
	{
		d = getenv("HOME");
		if (!d)
		{
			fputs("No home\n", stderr);
			return 1;
		}
	}
	else
		d = argv[1];
	
	if (chdir(d))
	{
		perror(d);
		return ERR_CHDIR;
	}
	return 0;
}

static int cmd_goto(int argc, char **argv)
{
	struct stat st;
	char *p;
	
	if (argc != 2)
	{
		fputs("goto: wrong nr of args\n", stderr);
		return ERR_ARGC;
	}
	
	if (fstat(fileno(source), &st))
	{
		perror(source_name);
		return ERR_STAT;
	}
	
	if (!S_ISREG(st.st_mode))
	{
		fprintf(stderr, "%s: Not a regular file\n", source_name);
		return ERR_SEEK;
	}
	
	rewind(source);
	while (p = readln(), p) /* XXX */
	{
		while (isspace(*p))
			p++;
		
		if (*p != ':')
			continue;
		p++;
		
		while (isspace(*p))
			p++;
		
		if (!strcmp(p, argv[1]))
			return 0;
	}
	
	fprintf(stderr, "goto: %s: Not found\n", argv[1]);
	return ERR_NOLABEL;
}

static int cmd_umask(int argc, char **argv)
{
	mode_t m;
	
	if (argc < 2)
	{
		m = umask(0777);
		umask(m);
		printf("0%03o\n", (unsigned)m);
		return 0;
	}
	if (argc > 2)
	{
		fputs("umask: wrong nr of args\n", stderr);
		return ERR_ARGC;
	}
	umask(otoi(argv[1]));
	return 0;
}

static int cmd_newgrp(int argc, char **argv)
{
	if (!logsh)
	{
		fputs("Not a login shell\n", stderr);
		return ERR_UNK;
	}
	execv("/bin/newgrp", argv);
	perror("/bin/newgrp");
	return ERR_EXEC;
}

static void unsetenv(char *name)
{
	int l = strlen(name);
	int i;
	
	for (i = 0; nenv[i]; i++)
		if (!strncmp(nenv[i], name, l) && nenv[i][l] == '=')
			break;
	if (!nenv[i])
		return;
	
	for (; nenv[i]; i++)
	{
		envdup[i] = envdup[i + 1];
		nenv[i] = nenv[i + 1];
	}
}

static int cmd_unset(int argc, char **argv)
{
	int i;
	
	for (i = 1; i < argc; i++)
		unsetenv(argv[i]);
	return 0;
}

static int cmd_exec(int argc, char **argv)
{
	execvp(argv[1], argv + 1);
	perror(argv[1]);
	return ERR_EXEC;
}

static int otoi(char *nptr)
{
	int n = 0;
	
	while (*nptr >= '0' && *nptr <= '7')
	{
		n <<= 3;
		n  += *nptr++ - '0';
	}
	return n;
}

static void prompt(char *s)
{
	write(1, s, strlen(s));
	strcpy(lastprompt, s);
}

static int waitexit(int st)
{
	if (WTERMSIG(st))
		return 128 + WTERMSIG(st);
	return WEXITSTATUS(st);
}

static char *readln(void)
{
	static char buf[4096];
	
	char *ps1;
	char *ps2;
	char *p;
	int c;
	
	ps1 = getenv("PS1");
	ps1 = ps1 ? ps1 : geteuid() ? "$ " : "# ";
	
	if (source == stdin)
		prompt(ps1);
	
again:
	if (!fgets(buf, sizeof buf, source))
	{
		if (ferror(source) && errno == EINTR)
		{
			clearerr(source);
			goto again;
		}
		if (ferror(source))
			perror(source_name);
		return NULL;
	}
	*lastprompt = 0;
	
	p = strchr(buf, '\n');
	if (p)
		*p = 0;
	
	return buf;
}

static void pstatus(int st)
{
	int sig = WTERMSIG(st);
	
	if (sig && sig != SIGINT)
		fprintf(stderr, "%s%s\n", sys_siglist[sig],
			WCOREDUMP(st) ? " (core dumped)" : "");
}

static char *svar(int c)
{
	if (isdigit(c))
	{
		c -= '0';
		if (!c)
		{
			if (source != stdin)
				return source_name;
			return arg0;
		}
		if (c >= sargc)
			return "";
		return sargv[c];
	}
	return ""; /* silence a warning */
}

static void subst1(char **dp, char **sp)
{
	static char buf[8];
	
	char *s = *sp;
	char *d = *dp;
	char *n;
	char *v;
	int c;
	
	switch (*s)
	{
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		v = svar(*s);
		(*sp)++;
		break;
	case '$':
		sprintf(buf, "%05u", (unsigned)getpid());
		v = buf;
		(*sp)++;
		break;
	case '?':
		sprintf(buf, "%i", lastst);
		v = buf;
		(*sp)++;
		break;
	default:
		for (n = s; *n; n++)
			if (isspace(*n) || ispunct(*n))
				break;
		c  = *n;
		*n = 0;
		v  = getenv(s);
		v  = v ? v : "";
		*sp  = n;
		*n   = c;
	}
	
	*dp += strlen(v);
	strcpy(d, v);
}

static void subst(char *s)
{
	char *d = sbuf;
	
	while (*s)
	{
		if (*s == '\\')
		{
			*d++ = *s++;
			if (*s)
				*d++ = *s++;
			continue;
		}
		if (*s == '$')
		{
			s++;
			subst1(&d, &s);
		}
		else
			*d++ = *s++;
	}
	*d = 0;
}

static char inp[] = "<";
static char out[] = ">";
static char err[] = "#";
static char pip[] = "|";
static char amp[] = "&";
static char app[] = ">>";

static char *stopcs(char **pp)
{
	char *p = *pp;
	int c = *p++;
	char *cs;
	
	*pp = p;
	
	switch (c)
	{
	case '<':
		return inp;
	case '>':
		if (*p == '>')
		{
			(*pp)++;
			return app;
		}
		return out;
	case '#':
		return err;
	case '|':
	case '^':
		return pip;
	case '&':
		return amp;
	default:
		abort();
		return 0; /* XXX silence a clang warning */
	}
}

static int isstop(int c)
{
	return (c == '&' || c == '|' || c == '^' || c == '<' || c == '>' || c == '#');
}

static void split(void)
{
	char *s, *p, *p1;
	int cnt = 1;
	int q = 0;
	int c;
	
	args[0] = "glob";
	
	for (s = sbuf; isspace(*s); s++)
		;
	
	while (*s && *s != ';')
	{
		if (isstop(*s))
		{
			args[cnt++] = stopcs(&s);
			continue;
		}
		
		if (isspace(*s))
		{
			s++;
			continue;
		}
		
		for (p = s; *p; p++)
		{
			if (*p == q)
			{
				q = 0;
				continue;
			}
			
			if (*p == '"' || *p == '\'')
			{
				q = *p;
				continue;
			}
			
			if (q)
				continue;
			
			if (isspace(*p))
				break;
			if (isstop(*p))
				break;
		}
		args[cnt++] = s;
		
		c = *p;
		p1 = p;
		if (isstop(c))
		{
			args[cnt++] = stopcs(&p);
			s = p;
			*p1 = 0;
			continue;
		}
		*p1 = 0;
		
		if (!c)
			break;
		s = p + 1;
	}
	args[cnt] = NULL;
}

static void redir1(int fd, char *path, int oflags)
{
	int nd;
	
	nd = open(path, oflags, 0666);
	if (nd < 0)
	{
		perror(path);
		exit(127);
	}
	dup2(nd, fd);
	close(nd);
}

static void redir(void)
{
	char **d;
	char **a;
	int i;
	
	for (d = a = args + 2; *a; a++)
	{
		if (*a == inp)
		{
			redir1(0, *++a, O_RDONLY);
			continue;
		}
		if (*a == out)
		{
			redir1(1, *++a, O_WRONLY | O_CREAT | O_TRUNC);
			continue;
		}
		if (*a == app)
		{
			redir1(1, *++a, O_WRONLY | O_CREAT | O_APPEND);
			continue;
		}
		if (*a == err)
		{
			redir1(2, *++a, O_WRONLY | O_CREAT | O_TRUNC);
			continue;
		}
		*d++ = *a;
	}
	*d = 0;
}

static void excom(void)
{
	int pi = 0;
	int g = 0;
	int i;
	char *p;
	
	for (i = 2; args[i]; i++)
	{
		for (p = args[i]; *p; p++)
			if (strchr(GLOB_CHRS, *p))
			{
				g = 1;
				break;
			}
		if (args[i] == pip)
		{
			args[i] = NULL;
			pi = i;
			break;
		}
	}
	
	signal(SIGTERM, SIG_DFL);
	
	if (pi)
	{
		int pfd[2];
		pid_t pid;
		
		if (pipe(pfd))
		{
			perror("pipe");
			_exit(ERR_EXEC);
		}
		
		pid = fork();
		if (!pid)
		{
			dup2(pfd[1], 1);
			excom();
		}
		if (pid < 0)
		{
			perror("fork");
			_exit(ERR_EXEC);
		}
		
		for (i = 1; args[pi + i]; i++)
			args[i] = args[pi + i];
		args[i] = NULL;
		
		dup2(pfd[0], 0);
		close(pfd[0]);
		close(pfd[1]);
		excom();
	}
	
	redir();
	
	if (g)
		globx(0, args);
	else
		execvp(args[1], args + 1);
	
	if (errno == ENOENT)
		fprintf(stderr, "%s: Command not found\n", args[1]);
	else
		perror(args[1]);
	_exit(ERR_EXEC);
}

static void putenv(char *s)
{
	char *p;
	int l;
	int i;
	
	p = strchr(s, '=');
	if (!p)
		abort();
	l = p - s + 1;
	
	s = strdup(s);
	if (!s)
	{
		fputs("Out of memory\n", stderr);
		return; // XXX
	}
	
	for (i = 0; i < MAXENV && nenv[i]; i++)
		if (!strncmp(nenv[i], s, l))
		{
			if (envdup[i])
				free(nenv[i]);
			goto set;
		}
	
	if (i >= MAXENV)
	{
		fputs("Too many variables\n", stderr);
		return; // XXX
	}
	envdup[i + 1] = NULL;
set:
	envdup[i] = 1;
	nenv[i] = s;
}

static void runcom(void)
{
	int nowait = 0;
	pid_t pid;
	int ac = 0;
	int st;
	int i;
	
	while (args[ac])
		ac++;
	
	if (args[ac - 1] == amp)
	{
		args[--ac] = NULL;
		nowait = 1;
	}
	
	if (ac < 2)
		return;
	
	if (ac == 2 && strchr(args[1], '='))
	{
		putenv(strdup(args[1]));
		return;
	}
	
	for (i = 0; i < sizeof comms / sizeof *comms; i++)
		if (!strcmp(comms[i].name, args[1]))
		{
			lastst = comms[i].proc(ac - 1, args + 1);
			return;
		}
	
	pid = fork();
	if (!pid)
	{
		if (nowait)
		{
			signal(SIGHUP, SIG_IGN);
			signal(SIGINT, SIG_IGN);
			signal(SIGQUIT, SIG_IGN);
		}
		excom();
	}
	if (pid < 0)
	{
		lastst = ERR_FORK;
		perror(NULL);
		return;
	}
	
	if (!nowait)
	{
		while (wait(&st) != pid)
			;
		lastst = waitexit(st);
		pstatus(st);
	}
}

static void quot(void)
{
	char *d, *s;
	char **a;
	
	for (a = args + 1; *a; a++)
	{
		for (d = s = *a; *s; s++)
			switch (*s)
			{
			case '\\':
				if (s[1])
					*d++ = *++s;
				break;
			case '\'':
			case '"':
				break;
			default:
				*d++ = *s;
			}
		*d = 0;
	}
}

static void docom(char *cmd)
{
	int i;
	
	subst(cmd);
	split();
	quot();
	runcom();
}

static void doline(char *ln)
{
	char *p, *s;
	int q;
	
	p = ln;
	while (isspace(*p))
		p++;
	if (*p == '#' || *p == ':')
		return;
	
	for (s = ln; *p; p++)
	{
		if (*p == '"' || *p == '\'')
		{
			for (q = *p++; *p; p++)
				if (*p == q)
					break;
		}
		
		if (*p == '\\')
		{
			if (p[1])
				p++;
			continue;
		}
		
		if (*p == ';')
		{
			*p = 0; docom(s); *p = ';';
			s = p + 1;
		}
	}
	docom(s);
}

static void loop(void)
{
	char *p;
	
again:
	while (p = readln(), p)
		doline(p);
	if (source_nxit)
	{
		source_name = "stdin";
		source_nxit = 0;
		source	    = stdin;
		goto again;
	}
}

static void profile(void)
{
	FILE *f;
	
	f = fopen(".profile", "r");
	if (f)
	{
		fcntl(fileno(f), F_SETFD, 1);
		source_name = ".profile";
		source_nxit = 1;
		source	    = f;
	}
}

static void siginth(int nr)
{
	if (*lastprompt)
	{
		write(1, "\n", 1);
		write(1, lastprompt, strlen(lastprompt));
	}
	
	signal(nr, siginth);
}

static void copyenv(void)
{
	int i;
	
	for (i = 0; i < MAXENV && environ[i]; i++)
		nenv[i] = environ[i];
	nenv[i] = NULL;
	
	if (environ[i])
		fputs("Too many variables\n", stderr);
	
	environ = nenv;
}

int main(int argc, char **argv)
{
	int s = 0;
	int c;
	
	source_name = "stdin";
	source	    = stdin;
	
	arg0 = argv[0];
	
	if (*argv[0] == '-')
		logsh = 1;
	
	if (argc > 2 && !strcmp(argv[1], "-c"))
	{
		doline(argv[2]);
		return lastst;
	}
	
	if (argc > 1)
	{
		source = fopen(argv[1], "r");
		source_name = argv[1];
		if (!source)
		{
			perror(argv[1]);
			return ERR_UNK;
		}
		fcntl(fileno(source), F_SETFD, 1);
		s = 1;
	}
	
	if (logsh)
		profile();
	
	if (!s)
	{
		sigquit = signal(SIGQUIT, siginth);
		sigterm = signal(SIGTERM, SIG_IGN);
		sigint  = signal(SIGINT,  siginth);
	}
	
	copyenv();
	loop();
	return lastst;
}
