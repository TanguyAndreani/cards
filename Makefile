CC=cc
STD=c99
CFLAGS=-g -Wall
BIN=a.out
SRC=cards.c csv.c
HEADERS=csv.h

$(BIN): $(SRC) $(HEADERS)
	$(CC) --std=$(STD) $(CFLAGS) -o $(BIN) $(SRC)
