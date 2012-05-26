# this is a comment
SRC=main.c
OBJ=$(SRC:.c=.o) # replaces the .c from SRC with .o
EXE=libusbtest.exe

CC=gcc
CFLAGS= -Wall -O3
LDFLAGS= -lusb-1.0
RM=rm

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY : all
all: $(EXE)

$(EXE): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $@

.PHONY : clean
clean:
	-$(RM) $(OBJ)