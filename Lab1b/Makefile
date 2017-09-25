## Makefile for Lab1a
## Christopher Bachelor
## cbachelor@ucla.edu
## UCLA ID: 004608570

CC=gcc

#Executables. Default makes all, or specify module
default: lab1a.c
	$(CC) lab1b-server.c -o lab1b-server
	$(CC) lab1b-client.c -o lab1b-client

lab1b-server: lab1b-server.c
	$(CC) lab1b-server.c -o lab1b-server

lab1b-client: lab1b-client.c
	$(CC) lab1b-client.c -o lab1b-client

#Make clean
.PHONY: clean
clean:
	rm -r lab1b-004608570.tar.gz lab1b-client lab1b-server

#Create tarball
submission_files = lab1b-server.c lab1b-client.c Makefile README my.key

dist:
	tar -czf lab1b-004608570.tar.gz $(submission_files)