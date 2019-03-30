CC=cc
STD=c99
CFLAGS=-g -Wall
BIN=a.out
SRC=cards.c

$(BIN): $(SRC)
	$(CC) --std=$(STD) $(CFLAGS) -o $(BIN) $(SRC)
