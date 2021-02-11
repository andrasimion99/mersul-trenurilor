all:
	gcc client.c -o client.exe
	gcc -Wall -I/usr/include/libxml2 server.c -o server.exe -lxml2
