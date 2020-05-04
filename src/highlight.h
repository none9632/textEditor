#pragma once

#include "row.h"

enum editorHighlight
{
	HL_NORMAL = 0,
	HL_COMMENT,
	HL_MLCOMMENT,
	HL_KEYWORD1,
	HL_KEYWORD2,
	HL_STRING,
	HL_NUMBER,
	HL_MATCH
};

typedef struct editorColor
{
	int R;
	int G;
	int B;
	int fgOrBg; // foreground or background
	int colorIndex;
} editorColor;

struct editorSyntax
{
	char *filetype;
	char **filematch;
	char **keywords;
	char *singleline_comment_start;
	char *multiline_comment_start;
	char *multiline_comment_end;
	int flags;
};

void editorUpdateSyntax(erow *row);
void editorSelectSyntaxHighlight();
editorColor *editorSyntaxToColor(int hl);