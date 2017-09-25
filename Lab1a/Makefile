## Makefile for Lab1a
## Christopher Bachelor
## cbachelor@ucla.edu
## UCLA ID: 004608570

CC=gcc

#Executable
lab1a: lab1a.c
	$(CC) lab1a.c -o lab1a 

#Make clean
.PHONY: clean
clean: 
	rm -r lab1a-004608570.tar.gz lab1a

#Create tarball
submission_files = lab1a.c Makefile README

dist:
	tar -czf lab1a-004608570.tar.gz $(submission_files)
