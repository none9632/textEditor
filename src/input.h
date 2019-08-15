#pragma once

char *editorPrompt(char *prompt, void (*callback)(char *, int));
void editorProcessKeypress();