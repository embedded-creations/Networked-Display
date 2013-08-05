CC=gcc
CFLAGS=-I/usr/local/include/ImageMagick/
LDFLAGS=-lvncclient -lpthread `pkg-config --cflags --libs MagickWand`
DEPS =
OBJ = viewer.o
PROJECTNAME = viewer

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: $(OBJ)
	gcc -o $(PROJECTNAME) $^ $(LDFLAGS)

clean:
	rm -f *.o *~ core
