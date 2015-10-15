

hashcache: hashcache.c sha2.[ch] hc.[ch]
	gcc -O2 -Wall -std=c99 -o hashcache hashcache.c sha2.c hc.c

