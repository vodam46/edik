CC=cc
OUT=edik
CLIBS=-lncurses
CFLAGS=-Wall -Wextra -pedantic

default: $(OUT)

debug: CFLAGS+=-g
debug:
	$(CC) $(CFLAGS) -lncurses edik.c -o $(OUT)
	gdb $(OUT)

$(OUT): edik.o
	$(CC) $(CLIBS) edik.o -o $(OUT)

clean:
	rm -Rf *.o $(OUT)

run: $(OUT)
	./$(OUT) $(ARGS)
