# MAKEFILE: Alexander Ling 430391570
all: myshell.c
	gcc -g -Wall -std=c99 -o myshell myshell.c

clean:
	$(RM) myshell