TARGET := text_editor

CC := gcc
CCFLAG := -Wall -g

OBJ_PATH := obj
SRC_PATH := src

vpath %.c $(SRC_PATH)
vpath %.o $(OBJ_PATH)

SRC_FILES := $(wildcard $(SRC_PATH)/*.c)
OBJ_FILES := $(subst $(SRC_PATH),$(OBJ_PATH),$(SRC_FILES:.c=.o))

$(TARGET): $(notdir $(OBJ_FILES))
	$(CC) $(CCFLAG) $(OBJ_FILES) -o $(TARGET)

%.o: %.c
	$(CC) $(CCFLAG) -c $< -o $(OBJ_PATH)/$@

rebuild: clean $(TARGET)

clean:
	rm -rf $(OBJ_PATH)/*.o $(TARGET)