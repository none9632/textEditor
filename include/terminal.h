#pragma once

void die(const char *s);
void enableRawMode();
int editorReadKey();
int getWindowSize(int *rows, int *cols);