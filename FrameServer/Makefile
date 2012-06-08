CC=gcc
CFLAGS=-I. -lvncserver -lpthread -lws2_32
DEPS = 
OBJ = frameserver.o 
PROJECTNAME = frameserver

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: $(OBJ)
	gcc -o $(PROJECTNAME).exe $^ $(CFLAGS)

clean:
	rm -f *.o *~ core 
