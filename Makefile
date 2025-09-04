all: rda

rda:
	gcc -I ./src -o rda ./src/rda.c

clean:
	rm -f ./rda
