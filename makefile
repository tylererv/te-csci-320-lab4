CC=gcc

translate: main.c address.c stat.c
	$(CC) $(CFLAGS) -o translate main.c address.c stat.c

clean:
	rm -f translate