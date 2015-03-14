all: client

server: client.c
	gcc -Wall -Werror client.c -o client -l../common/
	
clean:
	rm -f *.o
	rm -f *~
