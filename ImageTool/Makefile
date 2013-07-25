CC=gcc
CFLAGS=-I/usr/local/include/ImageMagick/ -lvncserver -lpthread `pkg-config --cflags --libs MagickWand`
DEPS =
OBJ = imagetool.o
PROJECTNAME = imagetool

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: $(OBJ)
	gcc -o $(PROJECTNAME) $^ $(CFLAGS)

clean:
	rm -f *.o *~ core
