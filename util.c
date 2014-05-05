/*
 * Netris -- A free networked version of T*tris
 * Copyright (C) 1994,1995,1996  Mark H. Weaver <mhw@netris.org>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: util.c,v 1.29 1999/05/16 06:56:33 mhw Exp $
 */

#include "netris.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netdb.h>
#include <errno.h>

static MyEventType AlarmGenFunc(EventGenRec *gen, MyEvent *event);

static EventGenRec alarmGen =
		{ &alarmGen, 0, FT_read, -1, AlarmGenFunc, EM_alarm };
static EventGenRec *nextGen = &alarmGen;

static myRandSeed = 1;

static struct timeval baseTimeval;

ExtFunc void InitUtil(void)
{
	if (initSeed)
		SRandom(initSeed);
	else
		SRandom(time(0));
	signal(SIGINT, CatchInt);
	ResetBaseTime();
}

ExtFunc void ResetBaseTime(void)
{
	gettimeofday(&baseTimeval, NULL);
}

ExtFunc void AtExit(void (*handler)(void))
{
#ifdef HAS_ON_EXIT
	on_exit((void *)handler, NULL);
#else
	atexit(handler);
#endif
}

ExtFunc void Usage(void)
{
	fprintf(stderr,
	  "Netris version %s (C) 1994-1996,1999  Mark H. Weaver <mhw@netris.org>\n"
	  "Usage: netris <options>\n"
	  "  -h		Print usage information\n"
	  "  -w		Wait for connection\n"
	  "  -c <host>	Initiate connection\n"
	  "  -p <port>	Set port number (default is %d)\n"
	  "  -k <keys>	Remap keys.  The argument is a prefix of the string\n"
	  "		  containing the keys in order: left, rotate, right, drop,\n"
	  "		  down-faster, toggle-spying, pause, faster, redraw.\n"
	  "		  \"^\" prefixes controls.  (default is \"%s\")\n"
	  "  -i <sec>	Set the step-down interval, in seconds\n"
	  "  -r <robot>	Execute <robot> (a command) as a robot controlling\n"
	  "		  the game instead of the keyboard\n"
	  "  -F		Use fair robot interface\n"
	  "  -s <seed>	Start with given random seed\n"
	  "  -D		Drops go into drop mode\n"
	  "		  This means that sliding off a cliff after a drop causes\n"
	  "		  another drop automatically\n"
	  "  -S		Disable inverse/bold/color for slow terminals\n"
	  "  -C		Disable color\n"
	  "  -H		Show distribution and warranty information\n"
	  "  -R		Show rules\n",
	  version_string, DEFAULT_PORT, DEFAULT_KEYS);
}

ExtFunc void DistInfo(void)
{
	fprintf(stderr,
	  "Netris version %s (C) 1994-1996,1999  Mark H. Weaver <mhw@netris.org>\n"
	  "\n"
	  "This program is free software; you can redistribute it and/or modify\n"
	  "it under the terms of the GNU General Public License as published by\n"
	  "the Free Software Foundation; either version 2 of the License, or\n"
	  "(at your option) any later version.\n"
	  "\n"
	  "This program is distributed in the hope that it will be useful,\n"
	  "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	  "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
	  "GNU General Public License for more details.\n"
	  "\n"
	  "You should have received a copy of the GNU General Public License\n"
	  "along with this program; if not, write to the Free Software\n"
	  "Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n",
	  version_string);
}

ExtFunc void Rules(void)
{
	fprintf(stderr,
	  "Netris version %s rules\n"
	  "\n"
	  "Two player mode\n"
	  "---------------\n"
	  "It's just like normal T*tris except that when you clear more than\n"
	  "one row with a single piece, the other player's board is moved up\n"
	  "and junk rows are added to the bottom.  If you clear 2, 3 or 4\n"
	  "rows, 1, 2 or 4 junk rows are added to your opponent's board,\n"
	  "respectively.  The junk rows have exactly one empty column.\n"
	  "For each group of junk rows given, the empty columns will line\n"
	  "up.  This is intentional.\n"
	  "\n"
	  "The longest surviving player wins the game.\n"
	  "\n"
	  "One player mode\n"
	  "---------------\n"
	  "This mode is currently very boring, because there's no scoring\n"
	  "and it never gets any faster.  This will be rectified at some point.\n"
	  "I'm not very motivated to do it right now because I'm sick of one\n"
	  "player T*tris.  For now, use the \"f\" key (by default) to make the\n"
	  "game go faster.  Speedups cannot be reversed for the remainder of\n"
	  "the game.\n",
	  version_string);
}

/*
 * My really crappy random number generator follows
 * Should be more than sufficient for our purposes though
 */
ExtFunc void SRandom(int seed)
{
	initSeed = seed;
	myRandSeed = seed % 31751 + 1;
}

ExtFunc int Random(int min, int max1)
{
	myRandSeed = (myRandSeed * 31751 + 15437) % 32767;
	return myRandSeed % (max1 - min) + min;
}

ExtFunc int MyRead(int fd, void *data, int len)
{
	int result, left;

	left = len;
	while (left > 0) {
		result = read(fd, data, left);
		if (result > 0) {
			data = ((char *)data) + result;
			left -= result;
		}
		else if (errno != EINTR)
			return result;
	}
	return len;
}

ExtFunc int MyWrite(int fd, void *data, int len)
{
	int result, left;

	left = len;
	while (left > 0) {
		result = write(fd, data, left);
		if (result > 0) {
			data = ((char *)data) + result;
			left -= result;
		}
		else if (errno != EINTR)
			return result;
	}
	return len;
}

ExtFunc void NormalizeTime(struct timeval *tv)
{
	tv->tv_sec += tv->tv_usec / 1000000;
	tv->tv_usec %= 1000000;
	if (tv->tv_usec < 0) {
		tv->tv_usec += 1000000;
		--tv->tv_sec;
	}
}

ExtFunc void CatchInt(int sig)
{
	exit(0);
}

ExtFunc void CatchAlarm(int sig)
{
	alarmGen.ready = 1;
	signal(SIGALRM, CatchAlarm);
}

static MyEventType AlarmGenFunc(EventGenRec *gen, MyEvent *event)
{
	return E_alarm;
}

ExtFunc long CurTimeval(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	tv.tv_sec -= baseTimeval.tv_sec;
	tv.tv_usec -= baseTimeval.tv_usec;
	return GetTimeval(&tv);
}

ExtFunc void SetTimeval(struct timeval *tv, long usec)
{
	tv->tv_sec = usec / 1000000;
	tv->tv_usec = usec % 1000000;
}

ExtFunc long GetTimeval(struct timeval *tv)
{
	return tv->tv_sec * 1000000 + tv->tv_usec;
}

static long SetITimer1(long interval, long value)
{
	struct itimerval it, old;

	SetTimeval(&it.it_interval, interval);
	SetTimeval(&it.it_value, value);
	if (setitimer(ITIMER_REAL, &it, &old) < 0)
		die("setitimer");
	signal(SIGALRM, CatchAlarm);
	return GetTimeval(&old.it_value);
}

ExtFunc long SetITimer(long interval, long value)
{
	long old;

	old = SetITimer1(0, 0);
	alarmGen.ready = 0;
	SetITimer1(interval, value);
	return old;
}

ExtFunc volatile void die(char *msg)
{
	perror(msg);
	exit(1);
}

ExtFunc volatile void fatal(char *msg)
{
	fprintf(stderr, "%s\n", msg);
	exit(1);
}

ExtFunc void BlockSignals(MySigSet *saved, ...)
{
	MySigSet set;
	va_list args;
	int sig;

	va_start(args, saved);
#ifdef HAS_SIGPROCMASK
	sigemptyset(&set);
#else
	set = 0;
#endif
	while ((sig = va_arg(args, int))) {
#ifdef HAS_SIGPROCMASK
		sigaddset(&set, sig);
#else
		sig |= sigmask(sig);
#endif
	}
#ifdef HAS_SIGPROCMASK
	sigprocmask(SIG_BLOCK, &set, saved);
#else
	*saved = sigblock(set);
#endif
	va_end(args);
}

ExtFunc void RestoreSignals(MySigSet *saved, MySigSet *set)
{
#ifdef HAS_SIGPROCMASK
	sigprocmask(SIG_SETMASK, set, saved);
#else
	if (saved)
		*saved = sigsetmask(*set);
	else
		sigsetmask(*set);
#endif
}

ExtFunc void AddEventGen(EventGenRec *gen)
{
	assert(gen->next == NULL);
	gen->next = nextGen->next;
	nextGen->next = gen;
}

ExtFunc void RemoveEventGen(EventGenRec *gen)
{
	/* assert(gen->next != NULL);	/* Be more forgiving, for SIGINTs */
	if (gen->next) {
		while (nextGen->next != gen)
			nextGen = nextGen->next;
		nextGen->next = gen->next;
		gen->next = NULL;
	}
}

ExtFunc MyEventType WaitMyEvent(MyEvent *event, int mask)
{
	int i, retry = 0;
	fd_set fds[FT_len];
	EventGenRec *gen;
	int result, anyReady, anySet;
	struct timeval tv;

	/* XXX In certain circumstances, this routine does polling */
	for (;;) {
		for (i = 0; i < FT_len; ++i)
			FD_ZERO(&fds[i]);
		anyReady = anySet = 0;
		gen = nextGen;
		do {
			if (gen->mask & mask) {
				if (gen->ready)
					anyReady = 1;
				if (gen->fd >= 0) {
					FD_SET(gen->fd, &fds[gen->fdType]);
					anySet = 1;
				}
			}
			gen = gen->next;
		} while (gen != nextGen);
		if (anySet) {
			tv.tv_sec = 0;
			tv.tv_usec = (retry && !anyReady) ? 500000 : 0;
			result = select(FD_SETSIZE, &fds[FT_read], &fds[FT_write],
					&fds[FT_except], anyReady ? &tv : NULL);
		}
		else {
			if (retry && !anyReady)
				sleep(1);
			result = 0;
		}
		gen = nextGen;
		do {
			if ((gen->mask & mask)
					&& (gen->ready || (result > 0 && gen->fd >= 0
						&& FD_ISSET(gen->fd, &fds[gen->fdType])))) {
				gen->ready = 0;
				event->type = gen->func(gen, event);
				if (event->type != E_none) {
					nextGen = gen->next;
					return event->type;
				}
			}
			gen = gen->next;
		} while (gen != nextGen);
		retry = 1;
	}
}

/*
 * vi: ts=4 ai
 * vim: noai si
 */
