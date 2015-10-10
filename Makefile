

hashcache: hashcache.c sha2.c sha2.h
	gcc -O2 -Wall -std=c99 -o hashcache hashcache.c sha2.c

