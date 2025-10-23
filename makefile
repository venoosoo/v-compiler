# Makefile for your project

CC = gcc
CFLAGS = -Wall -g
OUT = main

# Source files
SRC = main.c \
      parser/parser.c \
      parser/binstmt/binstmt.c \
      tokenizer/tokenizer.c \
      generation/generation.c \
      generation/helper/helper.c \
      libs/sds.c  

# Object files
OBJ = $(SRC:.c=.o)

# Default target
all: $(OUT)

# Link the executable
$(OUT): $(OBJ)
	$(CC) $(OBJ) -o $(OUT)

# Compile each .c to .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(OBJ) $(OUT) v
