
main: *.c lib/*.c
	gcc -std=c99 -o $@ $^ -Wall -pedantic -g

