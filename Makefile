CC=cc
STD=c99
BIN=a.out
SRC=cards.c

$(BIN): $(SRC)
	$(CC) --std=$(STD) $(SRC)
