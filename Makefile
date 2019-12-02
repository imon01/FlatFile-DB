BINDIR := $(shell pwd)
CC := gcc
SRC := main.c db.c
CFLAGS := -std=c99
PROG := dbcli

all: $(PROG)


$(PROG): $(SRC)
	$(CC) $(SRC) -o $@ 


.PHONEY: clean
.PHONEY: setup 

clean: 
	rm $(PROG)

setup:
#	$(shell export PATH=${PATH}:$(BINDIR))
 
