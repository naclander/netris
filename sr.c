/*
 * sr -- A sample robot for Netris
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
 * $Id: sr.c,v 1.10 1996/02/09 08:22:20 mhw Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <limits.h>

/* Both of these should be at least twice the actual max */
#define MAX_BOARD_WIDTH		32
#define MAX_BOARD_HEIGHT	64

char b[1024];
FILE *logFile;

int twoPlayer;
int boardHeight, boardWidth;
int board[MAX_BOARD_HEIGHT][MAX_BOARD_WIDTH];
int piece[4][4];
int pieceLast[4][4];

int board1[MAX_BOARD_HEIGHT][MAX_BOARD_WIDTH];
int piece1[4][4];
int piece2[4][4];

int pieceCount;		/* Serial number of current piece, for sending commands */
int pieceVisible;	/* How many blocks of the current piece are visible */
int pieceBottom, pieceLeft;	/* Position of bottom-left square */
int pieceBottomLast, pieceLeftLast;

/*
 * 0 = Not decided yet
 * 1 = decided
 * 2 = move in progress
 * 3 = drop in progress
 */
int pieceState;

int leftDest;
int pieceDest[4][4];

int masterEnable = 1, dropEnable = 1;

float curTime, moveTimeout;

int min(int a, int b)
{
	return a < b ? a : b;
}

char *ReadLine(char *buf, int size)
{
	int len;

	if (!fgets(buf, size, stdin))
		return NULL;
	len = strlen(buf);
	if (len > 0 && buf[len-1] == '\n')
		buf[len-1] = 0;
	if (logFile)
		fprintf(logFile, "  %s\n", buf);
	return buf;
}

int WriteLine(char *fmt, ...)
{
	int result;
	va_list args;

	va_start(args, fmt);
	result = vfprintf(stdout, fmt, args);
	if (logFile) {
		fprintf(logFile, "> ");
		vfprintf(logFile, fmt, args);
	}
	va_end(args);
	return result;
}

void FindPiece(void)
{
	int row, col;

	pieceVisible = 0;
	pieceBottom = MAX_BOARD_HEIGHT;
	pieceLeft = MAX_BOARD_WIDTH;
	for (row = boardHeight - 1; row >= 0; --row)
		for (col = boardWidth - 1; col >= 0; --col)
			if (board[row][col] < 0) {
				pieceBottom = row;
				if (pieceLeft > col)
					pieceLeft = col;
				pieceVisible++;
			}
	for (row = 0; row < 4; ++row)
		for (col = 0; col < 4; ++col)
			piece[row][col] = board[pieceBottom + row][pieceLeft + col] < 0;
}

void RotatePiece1(void)
{
	int row, col, height = 0;

	for (row = 0; row < 4; ++row)
		for (col = 0; col < 4; ++col)
		{
			piece2[row][col] = piece1[row][col];
			piece1[row][col] = 0;
			if (piece2[row][col])
				height = row + 1;
		}
	for (row = 0; row < 4; ++row)
		for (col = 0; col < height; ++col)
			piece1[row][col] = piece2[height - col - 1][row];
}

int PieceFits(int row, int col)
{
	int i, j;

	if (row < 0)
		return 0;
	for (i = 0; i < 4; ++i)
		for (j = 0; j < 4; ++j)
			if (piece1[i][j])
				if (col+j >= boardWidth || board[row+i][col+j] > 0)
					return 0;
	return 1;
}

int SimPlacement(int row, int col)
{
	int i, j;
	int from, to, count;

	for (i = 0; i < boardHeight; ++i)
		for (j = 0; j < boardWidth; ++j) {
			board1[i][j] = board[i][j] > 0;
			if (i >= row && i < row+4 && j >= col && j < col+4)
				if (piece1[i - row][j - col])
					board1[i][j] = 1;
		}
	for (from = to = 0; to < boardHeight; ++from) {
		count = boardWidth;
		for (j = 0; j < boardWidth; ++j)
			count -= board1[to][j] = board1[from][j];
		to += (count > 0);
	}
	return from - to;
}

double BoardScore(int linesCleared, int pRow, int verbose)
{
	double score = 0;
	double avgHeight2 = 0, avgHolesTimesDepth = 0;
	int maxMidHeight = 0, maxHeight = 0;
	int height[MAX_BOARD_WIDTH];
	int holesTimesDepth[MAX_BOARD_WIDTH];
	int holes[MAX_BOARD_WIDTH];
	double hardFit[MAX_BOARD_HEIGHT];
	int depend[MAX_BOARD_HEIGHT];
	int cover[MAX_BOARD_WIDTH];
	int row, col, count, i;
	int deltaLeft, deltaRight;
	double closeToTop, topShape = 0, fitProbs = 0, space = 0;
	double maxHard;

	for (col = 0; col < boardWidth; ++col) {
		cover[col] = 0;
		height[col] = 0;
		for (row = 0; row < boardHeight; ++row)
			if (board1[row][col])
				height[col] = row + 1;
		avgHeight2 += height[col] * height[col];
		if (maxHeight < height[col])
			maxHeight = height[col];
		if (col >= 2 && col < boardWidth - 2 && maxMidHeight < height[col])
			maxMidHeight = height[col];
		holes[col] = 0;
		holesTimesDepth[col] = 0;
		for (row = 0; row < height[col]; ++row) {
			if (board1[row][col])
				holesTimesDepth[col] += holes[col];
			else
				holes[col]++;
		}
		avgHolesTimesDepth += holesTimesDepth[col];
	}
	avgHeight2 /= boardWidth;
	avgHolesTimesDepth /= boardWidth;

	/* Calculate dependencies */
	for (row = maxHeight - 1; row >= 0; --row) {
		depend[row] = 0;
		for (col = 0; col < boardWidth; ++col) {
			if (board1[row][col])
				cover[col] |= 1 << row;
			else
				depend[row] |= cover[col];
		}
		for (i = row + 1; i < maxHeight; ++i)
			if (depend[row] & (1 << i))
				depend[row] |= depend[i];
	}

	/* Calculate hardness of fit */
	for (row = maxHeight - 1; row >= 0; --row) {
		hardFit[row] = 5;
		count = 0;
		for (col = 0; col < boardWidth; ++col) {
			if (board1[row][col]) {
				space += 0.5;
			}
			else if (!board1[row][col]) {
				count++;
				space += 1;
				hardFit[row]++;
				if (height[col] < row)
					hardFit[row] += row - height[col];
				if (col > 0)
					deltaLeft = height[col - 1] - row;
				else
					deltaLeft = MAX_BOARD_HEIGHT;
				if (col < boardWidth - 1)
					deltaRight = height[col + 1] - row;
				else
					deltaRight = MAX_BOARD_HEIGHT;
				if (deltaLeft > 2 && deltaRight > 2)
					hardFit[row] += 7;
				else if (deltaLeft > 2 || deltaRight > 2)
					hardFit[row] += 2;
				else if (abs(deltaLeft) == 2 && abs(deltaRight) == 2)
					hardFit[row] += 2;
				else if (abs(deltaLeft) == 2 || abs(deltaRight) == 2)
					hardFit[row] += 3;
			}
		}
		maxHard = 0;
		for (i = row + 1; i < row + 5 && i < maxHeight; ++i)
			if (depend[row] & (1 << i))
				if (maxHard < hardFit[i])
					maxHard = hardFit[i];
		fitProbs += maxHard * count;
	}

	/* Calculate score based on top shape */
	for (col = 0; col < boardWidth; ++col) {
		if (col > 0)
			deltaLeft = height[col - 1] - height[col];
		else
			deltaLeft = MAX_BOARD_HEIGHT;
		if (col < boardWidth - 1)
			deltaRight = height[col + 1] - height[col];
		else
			deltaRight = MAX_BOARD_HEIGHT;
		if (deltaLeft > 2 && deltaRight > 2)
			topShape += 15 + 15 * (min(deltaLeft, deltaRight) / 4);
		else if (deltaLeft > 2 || deltaRight > 2)
			topShape += 2;
		else if (abs(deltaLeft) == 2 && abs(deltaRight) == 2)
			topShape += 2;
		else if (abs(deltaLeft) == 2 || abs(deltaRight) == 2)
			topShape += 3;
	}

	closeToTop = (pRow / (double)boardHeight);
	closeToTop *= closeToTop;

	closeToTop *= 200;
	space /= 2;
	score = space + closeToTop + topShape + fitProbs - linesCleared * 10;

	if (verbose) {
		WriteLine("Message space=%g, close=%g, shape=%g\n",
			space, closeToTop, topShape);
		WriteLine("Message fitProbs=%g, cleared=%d\n",
			fitProbs, -linesCleared * 10);
	}

	return score;
}

void PrintGoal(void)
{
	char b[32];
	int i, j, c;

	c = 0;
	for (i = 0; i < 4; ++i) {
		b[c++] = ':';
		for (j = 0; j < 4; ++j)
			b[c++] = pieceDest[i][j] ? '*' : ' ';
	}
	b[c++]=':';
	b[c++]=0;
	WriteLine("Message Goal %d %s\n", leftDest, b);
}

double MakeDecision(void)
{
	int row, col, rot;
	int linesCleared;
	int first = 1;
	double minScore = 0, score;

	memcpy(piece1, piece, sizeof(piece));
	for (rot = 0; rot < 4; ++rot) {
		RotatePiece1();
		for (col = 0; col < boardWidth; ++col) {
			if (!PieceFits(pieceBottom, col))
				continue;
			for (row = pieceBottom; PieceFits(row-1, col); --row)
				;
			linesCleared = SimPlacement(row, col);
			score = BoardScore(linesCleared, row, 0);
			if (first || minScore > score) {
				first = 0;
				minScore = score;
				memcpy(pieceDest, piece1, sizeof(piece));
				leftDest = col;
			}
		}
	}
	PrintGoal();
	return minScore;
}

double PeekScore(int verbose)
{
	int row, col, linesCleared;

	memcpy(piece1, piece, sizeof(piece));
	col = pieceLeft;
	for (row = pieceBottom; PieceFits(row-1, col); --row)
		;
	linesCleared = SimPlacement(row, col);
	return BoardScore(linesCleared, row, verbose);
}

int main(int argc, char **argv)
{
	int ac;
	char *av[32];

	if (argc == 2 && !strcmp(argv[1], "-l")) {
		logFile = fopen("log", "w");
		if (!logFile) {
			perror("fopen log");
			exit(1);
		}
	}
	setvbuf(stdout, NULL, _IOLBF, 0);
	WriteLine("Version 1\n");
	while(ReadLine(b, sizeof b)) {
		av[0] = strtok(b, " ");
		if (!av[0])
			continue;
		ac = 1;
		while ((av[ac] = strtok(NULL, " ")))
			ac++;
		if (!strcmp(av[0], "Exit"))
			return 0;
		else if (!strcmp(av[0], "NewPiece") && ac >= 2) {
			pieceCount = atoi(av[1]);
			pieceState = 0;
		}
		else if (!strcmp(av[0], "BoardSize") && ac >= 4) {
			if (atoi(av[1]) != 0)
				continue;
			boardHeight = atoi(av[2]);
			boardWidth = atoi(av[3]);
		}
		else if (!strcmp(av[0], "RowUpdate") && ac >= 3 + boardWidth) {
			int scr, row, col;

			scr = atoi(av[1]);
			if (scr != 0)
				continue;
			row = atoi(av[2]);
			for (col = 0; col < boardWidth; col++)
				board[row][col] = atoi(av[3 + col]);
		}
		else if (!strcmp(av[0], "UserKey") && ac >= 3) {
			char key;

			key = atoi(av[1]);
			switch (key) {
				case 'v':
				case 's':
					FindPiece();
					WriteLine("Message Score = %g\n", PeekScore(key == 'v'));
					break;
				case 'e':
					masterEnable = !masterEnable;
					WriteLine("Message Enable = %d\n", masterEnable);
					break;
				case 'd':
					dropEnable = !dropEnable;
					WriteLine("Message Drop Enable = %d\n", dropEnable);
					break;
				default:
					if (strcmp(av[2], "?"))
						WriteLine("%s %d\n", av[2], pieceCount);
					break;
			}
		}
		else if (!strcmp(av[0], "TimeStamp") && ac >= 2 && masterEnable) {
			curTime = atof(av[1]);
			FindPiece();
			if (pieceVisible < 4)
				continue;
			if (memcmp(piece, pieceLast, sizeof(piece)) ||
					pieceLeft != pieceLeftLast) {
				if (pieceState == 2)
					pieceState = 1;
				memcpy(pieceLast, piece, sizeof(piece));
				pieceLeftLast = pieceLeft;
			}
			if (pieceState == 0) {		/* Undecided */
				MakeDecision();
				pieceState = 1;
			}
			if (pieceState >= 2) {		/* Move or drop in progress */
				if (curTime >= moveTimeout)
					pieceState = 1;
			}
			if (pieceState == 1) {		/* Decided */
				if (memcmp(piece, pieceDest, sizeof(piece))) {
					WriteLine("Rotate %d\n", pieceCount);
					pieceState = 2;
				}
				else if (pieceLeft != leftDest) {
					if (pieceLeft < leftDest)
						WriteLine("Right %d\n", pieceCount);
					else
						WriteLine("Left %d\n", pieceCount);
					pieceState = 2;
				}
				else if (dropEnable) {
					WriteLine("Drop %d\n", pieceCount);
					pieceState = 3;
				}
				if (pieceState == 2)
					moveTimeout = curTime + 0.5;
			}
		}
	}
	return 0;
}

/*
 * vi: ts=4 ai
 * vim: noai si
 */
