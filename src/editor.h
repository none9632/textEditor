#pragma once

#include <time.h>
#include <termios.h>

#include "row.h"

enum editorKey
{
	BACKSPACE = 127,
	ARROW_LEFT = 1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	DEL_KEY,
	HOME_KEY,
	END_KEY,
	PAGE_UP,
	PAGE_DOWN
};

struct editorConfig
{
	int cx, cy;
	int rx;
	int rowoff;
	int coloff;
	int screenrows;
	int screencols;
	int prev_screenrows;
	int prev_screencols;
	int numrows;
	int volnum;
	erow *row;
	int dirty;
	char *filename;
	char statusmsg[80];
	time_t statusmsg_time;
	struct editorSyntax *syntax;
	struct termios orig_termios;
};