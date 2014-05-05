/*
 * Netris -- A free networked version of T*tris
 * Copyright (C) 1994-1996,1999  Mark H. Weaver <mhw@netris.org>
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
 * $Id: netris.h,v 1.28 1999/05/16 06:56:29 mhw Exp $
 */

#ifndef NETRIS_H
#define NETRIS_H

#include "config.h"
#include <sys/time.h>
#include <assert.h>
#include <stdio.h>
#include <signal.h>

#define ExtFunc		/* Marks functions that need prototypes */

#ifdef NOEXT
# define EXT
# define IN(a) a
#else
# define EXT extern
# define IN(a)
#endif

#ifndef NULL
# define NULL ((void *)0)
#endif

#ifdef HAS_SIGPROCMASK
typedef sigset_t MySigSet;
#else
typedef int MySigSet;
#endif

/*
 * The following definitions are to ensure network compatibility even if
 * the sizes of ints and shorts are different.  I'm not sure exactly how
 * to deal with this problem, so I've added an abstraction layer.
 */

typedef short netint2;
typedef long netint4;

#define hton2(x) htons(x)
#define hton4(x) htonl(x)

#define ntoh2(x) ntohs(x)
#define ntoh4(x) ntohl(x)

#define DEFAULT_PORT 9284	/* Very arbitrary */

#define DEFAULT_KEYS "jkl mspf^l"

/* Protocol versions */
#define MAJOR_VERSION		1	
#define PROTOCOL_VERSION	3
#define ROBOT_VERSION		1

#define MAX_BOARD_WIDTH		32
#define MAX_BOARD_HEIGHT	64
#define MAX_SCREENS			2

#define DEFAULT_INTERVAL	300000	/* Step-down interval in microseconds */

/* NP_startConn flags */
#define SCF_usingRobot		000001
#define SCF_fairRobot		000002
#define SCF_setSeed			000004

/* Event masks */
#define EM_alarm			000001
#define EM_key				000002
#define EM_net				000004
#define EM_robot			000010
#define EM_any				000777

typedef enum _GameType { GT_onePlayer, GT_classicTwo, GT_len } GameType;
typedef enum _BlockTypeA { BT_none, BT_white, BT_blue, BT_magenta,
							BT_cyan, BT_yellow, BT_green, BT_red,
							BT_wall, BT_len } BlockTypeA;
typedef enum _Dir { D_down, D_right, D_up, D_left } Dir;
typedef enum _Cmd { C_end, C_forw, C_back, C_left, C_right, C_plot } Cmd;
typedef enum _FDType { FT_read, FT_write, FT_except, FT_len } FDType;
typedef enum _MyEventType { E_none, E_alarm, E_key, E_net,
							E_lostConn, E_robot, E_lostRobot } MyEventType;
typedef enum _NetPacketType { NP_endConn, NP_giveJunk, NP_newPiece,
							NP_down, NP_left, NP_right,
							NP_rotate, NP_drop, NP_clear,
							NP_insertJunk, NP_startConn,
							NP_userName, NP_pause, NP_version,
							NP_byeBye } NetPacketType;

typedef signed char BlockType;

typedef struct _MyEvent {
	MyEventType type;
	union {
		char key;
		struct {
			NetPacketType type;
			int size;
			void *data;
		} net;
		struct {
			int size;
			char *data;
		} robot;
	} u;
} MyEvent;

struct _EventGenRec;
typedef MyEventType (*EventGenFunc)(struct _EventGenRec *gen, MyEvent *event);

typedef struct _EventGenRec {
	struct _EventGenRec *next;
	int ready;
	FDType fdType;
	int fd;
	EventGenFunc func;
	int mask;
} EventGenRec;

typedef struct _Shape {
	struct _Shape *rotateTo;
	int initY, initX, mirrored;
	Dir initDir;
	BlockType type;
	Cmd *cmds;
} Shape;

typedef struct _ShapeOption {
	float weight;
	Shape *shape;
} ShapeOption;

typedef int (*ShapeDrawFunc)(int scr, int y, int x,
					BlockType type, void *data);

EXT GameType game;
EXT int boardHeight[MAX_SCREENS];
EXT int boardVisible[MAX_SCREENS], boardWidth[MAX_SCREENS];
EXT Shape *curShape[MAX_SCREENS];
EXT int curY[MAX_SCREENS], curX[MAX_SCREENS];
EXT char opponentName[16], opponentHost[256];
EXT int standoutEnable, colorEnable;
EXT int robotEnable, robotVersion, fairRobot;
EXT int protocolVersion;

EXT long initSeed;
EXT long stepDownInterval, speed;

EXT int myFlags, opponentFlags;

EXT char scratch[1024];

extern ShapeOption stdOptions[];
extern char *version_string;

#include "proto.h"

#endif /* NETRIS_H */

/*
 * vi: ts=4 ai
 * vim: noai si
 */
