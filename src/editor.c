/*** includes ***/

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "editor.h"
#include "row.h"
#include "highlight.h"
#include "terminal.h"
#include "editorOp.h"
#include "fileio.h"
#include "output.h"
#include "input.h"

struct editorConfig E;

/*** init ***/

void initEditor()
{
	E.cx = 0;
	E.cy = 0;
	E.rx = 0;
	E.rowoff = 0;
	E.coloff = 0;
	E.numrows = 0;
	E.row = NULL;
	E.dirty = 0;
	E.filename = NULL;
	E.statusmsg[0] = '\0';
	E.statusmsg_time = 0;
	E.syntax = NULL;

	if (getWindowSize(&E.screenrows, &E.screencols) == -1)
		die("getWindowSize");
	E.screenrows -= 2;
}

int main(int argc, char *argv[])
{
	enableRawMode();
	initEditor();
	if (argc >= 2)
		editorOpen(argv[1]);

	editorSetStatusMessage("HELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-F = find");

	while (1)
	{
		editorRefreshScreen();
		editorProcessKeypress();
	}

	return 0;
}