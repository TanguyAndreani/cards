CC=cc
CFLAGS=-g -Wall
NAME=cards
SRC=cards.c csv.c
OBJ=$(SRC:.c=.o)

$(NAME): $(OBJ)
