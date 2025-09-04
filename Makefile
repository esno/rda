all: rda

rda:
	gcc -I ./src -o rda ./src/rda.c
