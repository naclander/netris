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
 * $Id: robot.c,v 1.8 1996/02/09 08:22:15 mhw Exp $
 */

#include "netris.h"
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

static MyEventType RobotGenFunc(EventGenRec *gen, MyEvent *event);

static EventGenRec robotGen =
		{ NULL, 0, FT_read, -1, RobotGenFunc, EM_robot };

static int robotProcess;
static FILE *toRobot;
static int toRobotFd, fromRobotFd;

static char robotBuf[128];
static int robotBufSize, robotBufMsg;

static int gotSigPipe;

ExtFunc void InitRobot(char *robotProg)
{
	int to[2], from[2];
	int status;
	MyEvent event;

	signal(SIGPIPE, CatchPipe);
	AtExit(CloseRobot);
	if (pipe(to) || pipe(from))
		die("pipe");
	robotProcess = fork();
	if (robotProcess < 0)
		die("fork");
	if (robotProcess == 0) {
		dup2(to[0], STDIN_FILENO);
		dup2(from[1], STDOUT_FILENO);
		close(to[0]);
		close(to[1]);
		close(from[0]);
		close(from[1]);
		execl("/bin/sh", "sh", "-c", robotProg, NULL);
		die("execl failed");
	}
	close(to[0]);
	close(from[1]);
	toRobotFd = to[1];
	robotGen.fd = fromRobotFd = from[0];
	if (!(toRobot = fdopen(toRobotFd, "w")))
		die("fdopen");
	if ((status = fcntl(fromRobotFd, F_GETFL, 0)) < 0)
		die("fcntl/F_GETFL");
	status |= O_NONBLOCK;
	if (fcntl(fromRobotFd, F_SETFL, status) < 0)
		die("fcntl/F_SETFL");
	AddEventGen(&robotGen);
	RobotCmd(1, "Version %d\n", ROBOT_VERSION);
	if (WaitMyEvent(&event, EM_robot) != E_robot)
		fatal("Robot didn't start successfully");
	if (1 > sscanf(event.u.robot.data, "Version %d", &robotVersion)
			|| robotVersion < 1)
		fatal("Invalid Version line from robot");
	if (robotVersion > ROBOT_VERSION)
		robotVersion = ROBOT_VERSION;
}

ExtFunc void CatchPipe(int sig)
{
	robotGen.ready = gotSigPipe = 1;
}

ExtFunc void RobotCmd(int flush, char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vfprintf(toRobot, fmt, args);
	va_end(args);
	if (flush)
		fflush(toRobot);
}

ExtFunc void RobotTimeStamp(void)
{
	RobotCmd(1, "TimeStamp %.3f\n", CurTimeval() / 1.0e6);
}

ExtFunc void CloseRobot(void)
{
	RemoveEventGen(&robotGen);
	if (robotProcess > 0)
		RobotCmd(1, "Exit\n");
	fclose(toRobot);
	close(fromRobotFd);
}

static MyEventType RobotGenFunc(EventGenRec *gen, MyEvent *event)
{
	static int more;
	int result, i;
	char *p;

	if (gotSigPipe) {
		gotSigPipe = 0;
		robotGen.ready = more;
		return E_lostRobot;
	}
	if (robotBufMsg > 0) {
		/*
		 *	Grrrrrr!  SunOS 4.1 doesn't have memmove (or atexit)
		 *  I'm told some others have a broken memmove
		 *
		 *	memmove(robotBuf, robotBuf + robotBufMsg,
		 *			robotBufSize - robotBufMsg);
		 */
		for (i = robotBufMsg; i < robotBufSize; ++i)
			robotBuf[i - robotBufMsg] = robotBuf[i];

		robotBufSize -= robotBufMsg;
		robotBufMsg = 0;
	}
	if (more)
		more = 0;
	else {
		do {
			result = read(fromRobotFd, robotBuf + robotBufSize,
					sizeof(robotBuf) - robotBufSize);
		} while (result < 0 && errno == EINTR);
		if (result <= 0)
			return E_lostRobot;
		robotBufSize += result;
	}
	if (!(p = memchr(robotBuf, '\n', robotBufSize))) {
		if (robotBufSize >= sizeof(robotBuf))
			fatal("Line from robot is too long");
		return E_none;
	}
	*p = 0;
	robotBufMsg = p - robotBuf + 1;
	robotGen.ready = more = (memchr(robotBuf + robotBufMsg, '\n',
			robotBufSize - robotBufMsg) != NULL);
	event->u.robot.size = p - robotBuf;
	event->u.robot.data = robotBuf;
	return E_robot;
}

/*
 * vi: ts=4 ai
 * vim: noai si
 */
