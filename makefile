
smallsh: smallsh.c
	gcc -o smallsh -g smallsh.c

all: smallsh

clean:
	rm smallsh
