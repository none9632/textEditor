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
#include "abuf.h"
#include "terminal.h"
#include "editorOp.h"

struct editorConfig E;

/*** prototypes ***/

void editorSetStatusMessage(const char *fmt, ...);
void editorRefreshScreen();
char *editorPrompt(char *prompt, void (*callback)(char *, int));

/*** file i/o ***/

char *editorRowsToString(int *buflen)
{
	int totlen = 0;
	int j;
	for (j = 0; j < E.numrows; ++j)
		totlen += E.row[j].size + 1;
	*buflen = totlen;

	char *buf = malloc(totlen);
	char *p = buf;
	for (j = 0; j < E.numrows; ++j)
	{
		memcpy(p, E.row[j].chars, E.row[j].size);
		p += E.row[j].size;
		*p = '\n';
		p++;
	}

	return buf;
}

void editorOpen(char *filename)
{
	E.filename = strdup(filename);

	editorSelectSyntaxHighlight();

	FILE *fp = fopen(filename, "r");
	if (!fp)
		die("fopen");

	char *line = NULL;
	size_t linecap = 0;
	ssize_t linelen;
	while ((linelen = getline(&line, &linecap, fp)) != -1)
	{
		while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
			--linelen;
		editorInsertRow(E.numrows, line, linelen);
	}
	free(line);
	fclose(fp);
	E.dirty = 0;
}

void editorSave()
{
	if (E.filename == NULL)
	{
		E.filename = editorPrompt("Save as: %s (ESC to cancel)", NULL);
		if (E.filename == NULL)
		{
			editorSetStatusMessage("Save aborted");
			return;
		}
		editorSelectSyntaxHighlight();
	}

	int len;
	char *buf = editorRowsToString(&len);

	int fd = open(E.filename, O_RDWR | O_CREAT, 0644);
	if (fd != -1)
	{
		if (ftruncate(fd, len) != -1)
		{
			if (write(fd, buf, len) == len)
			{
				close(fd);
				free(buf);
				E.dirty = 0;
				editorSetStatusMessage("%d bytes written to disk", len);
				return;
			}
		}
		close(fd);
	}

	free(buf);
	editorSetStatusMessage("Can't save! I/O error: %s", strerror(errno));
}

/*** find ***/

void editorFindCallback(char *query, int key)
{
	static int last_match = -1;
	static int direction = 1;

	static int saved_hl_line;
	static char *saved_hl = NULL;

	if (saved_hl)
	{
		memcpy(E.row[saved_hl_line].hl, saved_hl, E.row[saved_hl_line].rsize);
		free(saved_hl);
		saved_hl = NULL;
	}

	if (key == '\r' || key == '\x1b')
	{
		last_match = -1;
		direction = 1;
		return;
	}
	else if (key == ARROW_RIGHT || key == ARROW_DOWN)
		direction = 1;
	else if (key == ARROW_LEFT || key == ARROW_UP)
		direction = -1;
	else
	{
		last_match = -1;
		direction = 1;
	}

	if (last_match == -1)
		direction = 1;
	int current = last_match;
	for (int i = 0; i < E.numrows; ++i)
	{
		current += direction;
		if (current == -1)
			current = E.numrows - 1;
		else if (current == E.numrows)
			current = 0;

		erow *row = &E.row[current];
		char *match = strstr(row->render, query);
		if (match)
		{
			last_match = current;
			E.cy = current;
			E.cx = editorRowRxToCx(row, match - row->render);
			E.rowoff = E.numrows;

			saved_hl_line = current;
			saved_hl = malloc(row->rsize);
			memcpy(saved_hl, row->hl, row->rsize);
			memset(&row->hl[match - row->render], HL_MATCH, strlen(query));
			break;
		}
	}
}

void editorFind()
{
	int saved_cx = E.cx;
	int saved_cy = E.cy;
	int saved_coloff = E.coloff;
	int saved_rowoff = E.rowoff;

	char *query = editorPrompt("Search: %s (Use ESC/Arrows/Enter)",
								editorFindCallback);

	if (query)
		free(query);
	else
	{
		E.cx = saved_cx;
		E.cy = saved_cy;
		E.coloff = saved_coloff;
		E.rowoff = saved_rowoff;
	}
}

/*** input ***/

char *editorPrompt(char *prompt, void (*callback)(char *, int))
{
	size_t bufsize = 128;
	char *buf = malloc(bufsize);

	size_t buflen = 0;
	buf[0] = '\0';

	while (1)
	{
		editorSetStatusMessage(prompt, buf);
		editorRefreshScreen();

		int c = editorReadKey();
		if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE)
		{
			if (buflen != 0)
				buf[--buflen] = '\0';
		}
		else if (c == '\x1b')
		{
			editorSetStatusMessage("");
			if (callback) callback(buf, c);
			free(buf);
			return NULL;
		}
		else if (c == '\r')
		{
			if (buflen != 0)
			{
				editorSetStatusMessage("");
				if (callback) callback(buf, c);
				return buf;
			}
		}
		else if (!iscntrl(c) && c < 128)
		{
			if (buflen == bufsize - 1)
			{
				bufsize *= 2;
				buf = realloc(buf, bufsize);
			}
			buf[buflen++] = c;
			buf[buflen] = '\0';
		}

		if (callback) callback(buf, c);
	}
}

void editorMoveCursor(int key)
{
	erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];

	switch (key)
	{
	case ARROW_LEFT:
		if (E.cx != 0)
			E.cx--;
		else if (E.cy > 0)
		{
			--E.cy;
			E.cx = E.row[E.cy].size;
		}
		break;
	case ARROW_RIGHT:
		if (row && E.cx < row->size)
			E.cx++;
		else if (row && E.cx == row->size)
		{
			E.cy++;
			E.cx = 0;
		}
		break;
	case ARROW_UP:
		if (E.cy != 0)
			E.cy--;
		break;
	case ARROW_DOWN:
		if (E.cy < E.numrows)
			E.cy++;
		break;
	}

	row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
	int rowlen = row ? row->size : 0;
	if (E.cx > rowlen)
		E.cx = rowlen;
}

void editorProcessKeypress()
{
	static int quit_times = KILO_QUIT_TIMES;

	int c = editorReadKey();

	switch (c)
	{
	case '\r':
		editorInsertNewline();
		break;

	case CTRL_KEY('q'):
		if (E.dirty && quit_times > 0)
		{
			editorSetStatusMessage("WARNING!!! File has unsaved changes. "
				"Press Ctrl-Q %d more times to quit.", quit_times);
			quit_times--;
			return;
		}
		write(STDOUT_FILENO, "\x1b[2J", 4);
		write(STDOUT_FILENO, "\x1b[H", 3);
		exit(0);
		break;

	case CTRL_KEY('s'):
		editorSave();
		break;

	case HOME_KEY:
		E.cx = 0;
		break;

	case END_KEY:
		if (E.cy < E.numrows)
			E.cx = E.row[E.cy].size;
		break;

	case CTRL_KEY('f'):
		editorFind();
		break;

	case BACKSPACE:
	case CTRL_KEY('h'):
	case DEL_KEY:
		if (c == DEL_KEY)
			editorMoveCursor(ARROW_RIGHT);
		editorDelChar();
		break;

	case PAGE_UP:
	case PAGE_DOWN:
		{
			if (c == PAGE_UP)
			{
				E.cy = E.rowoff;
			}
			else if (c == PAGE_DOWN)
			{
				E.cy = E.rowoff + E.screenrows - 1;
				if (E.cy > E.numrows) E.cy = E.numrows;
			}

			int times = E.screenrows;
			while (times--)
				editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
		}
		break;

	case ARROW_UP:
	case ARROW_DOWN:
	case ARROW_LEFT:
	case ARROW_RIGHT:
		editorMoveCursor(c);
		break;

	case CTRL_KEY('l'):
	case '\x1b':
		break;

	default:
		editorInsertChar(c);
		break;
	}

	quit_times = KILO_QUIT_TIMES;
}

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