/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)script.c	5.14 (Berkeley) %G%";
#endif /* not lint */

/*
 * script
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/signal.h>
#include <stdio.h>
#include <paths.h>

char	*shell;
FILE	*fscript;
int	master;
int	slave;
int	child;
int	subchild;
char	*fname;

struct	termios tt;
struct	winsize win;
int	lb;
int	l;
char	line[] = "/dev/ptyXX";
int	aflg;

main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind;
	int ch;
	void finish();
	char *getenv();

	while ((ch = getopt(argc, argv, "a")) != EOF)
		switch((char)ch) {
		case 'a':
			aflg++;
			break;
		case '?':
		default:
			fprintf(stderr, "usage: script [-a] [file]\n");
			exit(1);
		}
	argc -= optind;
	argv += optind;

	if (argc > 0)
		fname = argv[0];
	else
		fname = "typescript";
	if ((fscript = fopen(fname, aflg ? "a" : "w")) == NULL) {
		perror(fname);
		fail();
	}

	shell = getenv("SHELL");
	if (shell == NULL)
		shell = _PATH_BSHELL;

	(void) tcgetattr(0, &tt);
	(void) ioctl(0, TIOCGWINSZ, (char *)&win);
	if (openpty(&master, &slave, NULL, &tt, &win) == -1) {
		perror("openpty");
		exit(1);
	}

	printf("Script started, file is %s\n", fname);
	fixtty();	/* go raw */

	(void) signal(SIGCHLD, finish);
	child = fork();
	if (child < 0) {
		perror("fork");
		fail();
	}
	if (child == 0) {
		subchild = child = fork();
		if (child < 0) {
			perror("fork");
			fail();
		}
		if (child)
			dooutput();
		else
			doshell();
	}
	doinput();
}

doinput()
{
	register int cc;
	char ibuf[BUFSIZ];

	(void) fclose(fscript);
	while ((cc = read(0, ibuf, BUFSIZ)) > 0)
		(void) write(master, ibuf, cc);
	done();
}

#include <sys/wait.h>

void
finish()
{
	union wait status;
	register int pid;
	register int die = 0;

	while ((pid = wait3((int *)&status, WNOHANG, 0)) > 0)
		if (pid == child)
			die = 1;

	if (die)
		done();
}

dooutput()
{
	register int cc;
	time_t tvec, time();
	char obuf[BUFSIZ], *ctime();

	(void) close(0);
	tvec = time((time_t *)NULL);
	fprintf(fscript, "Script started on %s", ctime(&tvec));
	for (;;) {
		cc = read(master, obuf, sizeof (obuf));
		if (cc <= 0)
			break;
		(void) write(1, obuf, cc);
		(void) fwrite(obuf, 1, cc, fscript);
	}
	done();
}

doshell()
{

	close(master);
	(void) fclose(fscript);
	login_tty(slave);
	execl(shell, "sh", "-i", 0);
	perror(shell);
	fail();
}

fixtty()
{
	struct termios rtt;

	rtt = tt;
	cfmakeraw(&rtt);
	rtt.c_lflag &= ~ECHO;
	(void) tcsetattr(0, TCSAFLUSH, &rtt);
}

fail()
{

	(void) kill(0, SIGTERM);
	done();
}

done()
{
	time_t tvec, time();
	char *ctime();

	if (subchild) {
		tvec = time((time_t *)NULL);
		fprintf(fscript,"\nScript done on %s", ctime(&tvec));
		(void) fclose(fscript);
		(void) close(master);
	} else {
		(void) tcsetattr(0, TCSAFLUSH, &tt);
		printf("Script done, file is %s\n", fname);
	}
	exit(0);
}
