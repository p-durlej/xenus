int __libc_va_arg();
int _ctty();
int _dmesg();
int _exec();
int _exit();
int _iopl();
int _killf();
int _killu();
int _mfree();
int _newregion();
int _setcompat();
int _sysmesg();
int _uptime();
int abort();
int access();
int alarm();
int asctime();
int atoi();
int atol();
int brk();
int calloc();
int cfgetispeed();
int cfgetospeed();
int cfsetispeed();
int cfsetospeed();
int chdir();
int chmod();
int chown();
int chroot();
int clearerr();
int close();
int closedir();
int creat();
int ctime();
int dup();
int dup2();
int endgrent();
int endpwent();
int execl();
int execle();
int execlp();
int execv();
int execve();
int execvp();
int exit();
int fchmod();
int fchown();
int fclose();
int fcntl();
int fdopen();
int feof();
int ferror();
int fflush();
int fgetc();
int fgetgrent();
int fgetpwent();
int fgets();
int fileno();
int fileno();
int fopen();
int fork();
int fprintf();
int fputc();
int fputs();
int free();
int fseek();
int fstat();
int ftell();
int getcwd();
int getegid();
int getenv();
int geteuid();
int getgid();
int getgrent();
int getgrgid();
int getgrnam();
int getpassn();
int getpgrp();
int getpid();
int getppid();
int getpwent();
int getpwnam();
int getpwuid();
int gets();
int getuid();
int gmtime();
int inb();
int inl();
int insb();
int insl();
int insw();
int inw();
int ioctl();
int isalnum();
int isalpha();
int isatty();
int iscntrl();
int isdigit();
int islower();
int isprint();
int ispunct();
int isspace();
int isupper();
int isxdigit();
int kill();
int link();
int localtime();
int longjmp();
int lseek();
int malloc();
int md5();
int md5a();
int memccpy();
int memchr();
int memcmp();
int memcpy();
int memcpy();
int memmove();
int memset();
int memset();
int mkdir();
int mkfifo();
int mknod();
int mktime();
int mount();
int open();
int opendir();
int outb();
int outl();
int outsb();
int outsl();
int outsw();
int outw();
int pause();
int perror();
int pipe();
int printf();
int putpwent();
int puts();
int qsort();
int raise();
int read();
int readdir();
int realloc();
int reboot();
int remove();
int rename();
int rewind();
int rewinddir();
int rmdir();
int sbrk();
int seekdir();
int setbuf();
int setbuffer();
int setgid();
int setgrent();
int setjmp();
int setlinebuf();
int setpwent();
int setsid();
int setuid();
int setvbuf();
int signal();
int sleep();
int snprintf();
int sprintf();
int stat();
int statfs();
int stime();
int strcat();
int strchr();
int strcmp();
int strcpy();
int strcspn();
int strdup();
int strerror();
int strlen();
int strncat();
int strncmp();
int strncpy();
int strpbrk();
int strrchr();
int strspn();
int strtok();
int sync();
int system();
int tcgetattr();
int tcsetattr();
int telldir();
int time();
int timegm();
int times();
int ttyname();
int ttyname3();
int tzset();
int ulimit();
int umask();
int umount();
int uname();
int ungetc();
int unlink();
int utime();
int vfprintf();
int vprintf();
int vsnprintf();
int vsprintf();
int wait();
int waitpid();
int write();
int strsignal();
int ftime();
int _ftime();
int _sfilsys();
int _readwrite();

void *libtab[] =
{
	[1] = __libc_va_arg,
	[2] = _ctty,
	[3] = _dmesg,
	[4] = _exec,
	[5] = _exit,
	[6] = _iopl,
	[7] = _killf,
	[8] = _killu,
	[9] = _mfree,
	[10] = _newregion,
	[11] = _setcompat,
	[12] = _sysmesg,
	[13] = _uptime,
	[14] = abort,
	[15] = access,
	[16] = alarm,
	[17] = asctime,
	[18] = atoi,
	[19] = atol,
	[20] = brk,
	[21] = calloc,
	[22] = cfgetispeed,
	[23] = cfgetospeed,
	[24] = cfsetispeed,
	[25] = cfsetospeed,
	[26] = chdir,
	[27] = chmod,
	[28] = chown,
	[29] = chroot,
	[30] = clearerr,
	[31] = close,
	[32] = closedir,
	[33] = creat,
	[34] = ctime,
	[35] = dup,
	[36] = dup2,
	[37] = endgrent,
	[38] = endpwent,
	[39] = execl,
	[40] = execle,
	[41] = execlp,
	[42] = execv,
	[43] = execve,
	[44] = execvp,
	[45] = exit,
	[46] = fchmod,
	[47] = fchown,
	[48] = fclose,
	[49] = fcntl,
	[50] = fdopen,
	[51] = feof,
	[52] = ferror,
	[53] = fflush,
	[54] = fgetc,
	[55] = fgetgrent,
	[56] = fgetpwent,
	[57] = fgets,
	[58] = fileno,
	[59] = fileno,
	[60] = fopen,
	[61] = fork,
	[62] = fprintf,
	[63] = fputc,
	[64] = fputs,
	[65] = free,
	[66] = fseek,
	[67] = fstat,
	[68] = ftell,
	[69] = getcwd,
	[70] = getegid,
	[71] = getenv,
	[72] = geteuid,
	[73] = getgid,
	[74] = getgrent,
	[75] = getgrgid,
	[76] = getgrnam,
	[77] = getpassn,
	[78] = getpgrp,
	[79] = getpid,
	[80] = getppid,
	[81] = getpwent,
	[82] = getpwnam,
	[83] = getpwuid,
	[84] = gets,
	[85] = getuid,
	[86] = gmtime,
	[87] = inb,
	[88] = inl,
	[89] = insb,
	[90] = insl,
	[91] = insw,
	[92] = inw,
	[93] = ioctl,
	[94] = isalnum,
	[95] = isalpha,
	[96] = isatty,
	[97] = iscntrl,
	[98] = isdigit,
	[99] = islower,
	[100] = isprint,
	[101] = ispunct,
	[102] = isspace,
	[103] = isupper,
	[104] = isxdigit,
	[105] = kill,
	[106] = link,
	[107] = localtime,
	[108] = longjmp,
	[109] = lseek,
	[110] = malloc,
	[111] = md5,
	[112] = md5a,
	[113] = memccpy,
	[114] = memchr,
	[115] = memcmp,
	[116] = memcpy,
	[117] = memcpy,
	[118] = memmove,
	[119] = memset,
	[120] = memset,
	[121] = mkdir,
	[122] = mkfifo,
	[123] = mknod,
	[124] = mktime,
	[125] = mount,
	[126] = open,
	[127] = opendir,
	[128] = outb,
	[129] = outl,
	[130] = outsb,
	[131] = outsl,
	[132] = outsw,
	[133] = outw,
	[134] = pause,
	[135] = perror,
	[136] = pipe,
	[137] = printf,
	[138] = putpwent,
	[139] = puts,
	[140] = qsort,
	[141] = raise,
	[142] = read,
	[143] = readdir,
	[144] = realloc,
	[145] = reboot,
	[146] = remove,
	[147] = rename,
	[148] = rewind,
	[149] = rewinddir,
	[150] = rmdir,
	[151] = sbrk,
	[152] = seekdir,
	[153] = setbuf,
	[154] = setbuffer,
	[155] = setgid,
	[156] = setgrent,
	[157] = setjmp,
	[158] = setlinebuf,
	[159] = setpwent,
	[160] = setsid,
	[161] = setuid,
	[162] = setvbuf,
	[163] = signal,
	[164] = sleep,
	[165] = snprintf,
	[166] = sprintf,
	[167] = stat,
	[168] = statfs,
	[169] = stime,
	[170] = strcat,
	[171] = strchr,
	[172] = strcmp,
	[173] = strcpy,
	[174] = strcspn,
	[175] = strdup,
	[176] = strerror,
	[177] = strlen,
	[178] = strncat,
	[179] = strncmp,
	[180] = strncpy,
	[181] = strpbrk,
	[182] = strrchr,
	[183] = strspn,
	[184] = strtok,
	[185] = sync,
	[186] = system,
	[187] = tcgetattr,
	[188] = tcsetattr,
	[189] = telldir,
	[190] = time,
	[191] = timegm,
	[192] = times,
	[193] = ttyname,
	[194] = ttyname3,
	[195] = tzset,
	[196] = ulimit,
	[197] = umask,
	[198] = umount,
	[199] = uname,
	[200] = ungetc,
	[201] = unlink,
	[202] = utime,
	[203] = vfprintf,
	[204] = vprintf,
	[205] = vsnprintf,
	[206] = vsprintf,
	[207] = wait,
	[208] = waitpid,
	[209] = write,
	[210] = strsignal,
	[211] = ftime,
	[212] = _ftime,
	[213] = _sfilsys,
	[214] = _readwrite,
};