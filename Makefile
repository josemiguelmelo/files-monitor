all: monitor.c

	mkdir -p bin
	gcc -Wall monitor.c -o bin/monitor