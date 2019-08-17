CC           := gcc
TARGET       := editor
CFLAGS       := -g3
OBJDIR       := obj
OBJ          := editor.o highlight.o row.o abuf.o terminal.o editorOp.o \
	output.o input.o fileio.o find.o

remake: clear $(TARGET)

$(TARGET): $(OBJDIR) $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)
	@mv *.o $(OBJDIR)

$(OBJDIR): $(OBJDIR)
	@mkdir $(OBJDIR)

%.o: src/%.c
	$(CC) $(CFLAGS) -c $^

clear:
	rm -rf $(OBJDIR) $(TARGET)