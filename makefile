maprecat: helpers.h maprecat.c hexdumpe.c hexdumpe.h
	gcc -Wall -O2 -I. maprecat.c hexdumpe.c -o maprecat
