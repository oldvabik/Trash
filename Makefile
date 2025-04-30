CC = gcc
CFLAGS = -W -Werror -std=c11 -pedantic -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable
UNLINK_FLAGS = -fpic -shared

BUILD = ./build
SRC = ./src

unlink = $(BUILD)/unlink.so
unlinkc = $(SRC)/unlink.c
trash = $(BUILD)/trash
trashc = $(SRC)/trash.c

.PHONY: clean 

all: $(unlink) $(trash)
	
$(unlink): $(unlinkc)
	$(CC) $(UNLINK_FLAGS) $(CFLAGS) $(unlinkc) -o $(unlink)

$(trash): $(trashc)
	$(CC) $(CFLAGS) $(trashc) -o $(trash)
	
clean:
	rm -f $(unlink) $(trash)
