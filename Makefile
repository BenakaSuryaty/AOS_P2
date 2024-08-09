# Makefile for Project_2

CC = gcc
CFLAGS = -Wall -g -std=c99

# Directories
P4_DIR = P4_coord
P1_DIR = P1
P2_DIR = P2
P3_DIR = P3


# Executables
P4_EXEC = p4_coord
P1_EXEC = p1
P2_EXEC = p2
P3_EXEC = p3

# Source files
P4_SRC = $(P4_DIR)/process4.c
P1_SRC = $(P1_DIR)/process1.c
P2_SRC = $(P2_DIR)/process2.c
P3_SRC = $(P3_DIR)/process3.c

# Targets
all: $(P4_EXEC) $(P1_EXEC) $(P2_EXEC) $(P3_EXEC)

$(P4_EXEC): $(P4_SRC)
	$(CC) $(CFLAGS) -o $(P4_DIR)/$(P4_EXEC) $(P4_SRC)

$(P1_EXEC): $(P1_SRC)
	$(CC) $(CFLAGS) -o $(P1_DIR)/$(P1_EXEC) $(P1_SRC)

$(P2_EXEC): $(P2_SRC)
	$(CC) $(CFLAGS) -o $(P2_DIR)/$(P2_EXEC) $(P2_SRC)

$(P3_EXEC): $(P3_SRC)
	$(CC) $(CFLAGS) -o $(P3_DIR)/$(P3_EXEC) $(P3_SRC)

clean:
	rm -f $(P4_DIR)/$(P4_EXEC) \
		  $(P1_DIR)/$(P1_EXEC) \
		  $(P2_DIR)/$(P2_EXEC) \
		  $(P3_DIR)/$(P3_EXEC) \
	
.PHONY: all clean