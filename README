#
# Netris -- A free networked version of T*tris
# Copyright (C) 1994-1996,1999  Mark H. Weaver <mhw@netris.org>
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
# $Id: README,v 1.21 1999/05/16 06:56:22 mhw Exp $
#

This is an unfinished developmental version of Netris, a free
networked version of T*tris.  It is distributed under the terms
of the GNU General Public License, which is described in the
file "COPYING" included with this distribution.  For more
information about GNU and the Free Software Foundation,
check out <http://www.gnu.org/>.

In order to compile Netris you will need gcc.  You may be able to
compile it with another ANSI C compiler, but if you attempt this
you are on your own.

It's been built and tested on at least the following systems:

    GNU/Linux
    FreeBSD 2.1.5, 2.1.6, 2.2
    NetBSD 1.0, 1.1
    SunOS 4.1.1, 4.1.3
    Solaris 2.3, 2.4
    HP-UX

If Netris doesn't build on your favorite system "out-of-the-box",
I encourage you to mail me context diffs to fix the problem so I
can fold it into the next version.

Netris should build cleanly on 64-bit systems such as the Alpha,
although you might need to edit the definitions for netint{2,4},
hton{2,4}, and ntoh{2,4} in netris.h.  Alpha users, please let me know
how it goes, and send me diffs if needed!

See the FAQ in this directory if you have any problems.


FIXED IN VERSION 0.52
=====================
Fixed a buffer overflow vulnerability discovered by
Artur Byszko / bajkero <bajkero@security.hack.pl>


NEW IN VERSION 0.5
==================
Netris now specifically looks for ncurses and uses color if it's
available, unless the -C option is given.  Thanks to A.P.J. van Loo
<cobra@multiweb.nl> for providing code which these changes are
based on.


INSTALLATION
============
1. Run "./Configure" to create a Makefile and config.h appropriate
   for your system.  If you have problems running Configure with
   your /bin/sh, try "bash Configure".
2. Try "make"
3. Make sure "./netris" works properly
4. Copy "./netris" to the appropriate public directory

Try "./Configure -h" for more options


RUNNING
=======
To start a two-player game, do the following:
 1. Player 1 types "netris -w".  This means "wait for challenge".
 2. Player 2 types "netris -c <host>" where <host> is the hostname
    of Player 1.  This means "challenge".

To start a one-player game, run netris with no parameters.
One-player mode is a tad boring at the moment, because it never
gets any faster, and there's no scoring.  This will be rectified
at some point.  For now, use the "f" key (by default) to make the
game go faster.  Speedups cannot be reversed for the remainder of
the game.

Unlike standard T*tris, Netris gives you a little extra time after
dropping a piece before it solidifies.  This allows you to slide the
piece into a notch without waiting for it to fall the whole way down.
In fact, you can even slide it off a cliff and it'll start falling
again.  If you think it should automatically drop again in this case,
use the -D option.

The keys are:
 'j'    left
 'k'    rotate
 'l'    right
 Space	drop
 'm'    down faster
 's'	toggle spying on the other player
 'p'	pause
 'f' 	make game faster (irreversible)
 Ctrl-L	redraw the screen

To see usage information, type "netris -h".
To see distribution/warranty information, type "netris -H".
To see the rules, type "netris -R".
To use a port number other than the default, use the -p option.

You can remap the keys with "-k <keys>", where <keys> is a string
containing the keys in the order listed above.  The default is:
    netris -k "jkl mspf^l"

You needn't specify all of the keys, for example -k "asd" will only
change the main three keys.  "^x" notation can be used for control
characters.

The "m" key moves the falling piece down one block, in addition to the
usual step-down timer.  Use this in repetition when "drop" would go
too far but you don't want to wait for the piece of fall.


RUMORS
======
At some point I may implement a server that Netris players can connect
to to find other players with similar skill across the globe.

This version at least partially supports robots.  A rough description
of the protocol is in "robot_desc", and a sample robot is in sr.c.

The source code should be viewed with tab stops set every 4 columns,
eg, "less -x4 game.c".

# vi: tw=70 ai
