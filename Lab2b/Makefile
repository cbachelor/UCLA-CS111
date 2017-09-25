## Makefile for Lab2b
## Christopher Bachelor
## cbachelor@ucla.edu
## UCLA ID: 004608570

CC=gcc

lab2b_list_plots = lab2b_1.png lab2b_2.png lab2b_3.png lab2b_4.png lab2b_5.png
submission_files = lab2_list.c SortedList.c SortedList.h README Makefile lab2b_list.gp \
	 lab2b_list.csv

#Executables. Compile all programs
lab2_list: lab2_list.c SortedList.c SortedList.h
	$(CC) -g -pthread lab2_list.c SortedList.c -o lab2_list


profile: lab2_list 
	rm -f ./raw.gperf
	LD_PRELOAD=/u/ee/ugrad/bachelor/lib/libprofiler.so.0 \
	CPUPROFILE=./raw.gperf ./lab2_list --threads=12 --iterations=1000 --sync=s 
	pprof --text ./lab2_list ./raw.gperf > profile.out 
	pprof --list=thread_func ./lab2_list ./raw.gperf >>profile.out 
	rm -f ./raw.gperf


#Tests for the project
tests: lab2_list
#Tests for lab2b_1.png and lab2b_2.png, and partially 4 & 5
	./lab2_list --threads=1 --iterations=1000 --sync=m >>lab2b_list.csv
	./lab2_list --threads=2 --iterations=1000 --sync=m >>lab2b_list.csv
	./lab2_list --threads=4 --iterations=1000 --sync=m >>lab2b_list.csv
	./lab2_list --threads=8 --iterations=1000 --sync=m >>lab2b_list.csv
	./lab2_list --threads=12 --iterations=1000 --sync=m >>lab2b_list.csv
	./lab2_list --threads=16 --iterations=1000 --sync=m >>lab2b_list.csv
	./lab2_list --threads=24 --iterations=1000 --sync=m >>lab2b_list.csv
	
	./lab2_list --threads=1 --iterations=1000 --sync=s >>lab2b_list.csv
	./lab2_list --threads=2 --iterations=1000 --sync=s >>lab2b_list.csv
	./lab2_list --threads=4 --iterations=1000 --sync=s >>lab2b_list.csv
	./lab2_list --threads=8 --iterations=1000 --sync=s >>lab2b_list.csv
	./lab2_list --threads=12 --iterations=1000 --sync=s >>lab2b_list.csv
	./lab2_list --threads=16 --iterations=1000 --sync=s >>lab2b_list.csv
	./lab2_list --threads=24 --iterations=1000 --sync=s >>lab2b_list.csv

#Tests for lab2b_3.png
	-./lab2_list --threads=1 --iterations=1 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=1 --iterations=2 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=1 --iterations=4 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=1 --iterations=8 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=1 --iterations=16 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=4 --iterations=1 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=4 --iterations=2 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=4 --iterations=4 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=4 --iterations=8 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=4 --iterations=16 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=8 --iterations=1 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=8 --iterations=2 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=8 --iterations=4 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=8 --iterations=8 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=8 --iterations=16 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=12 --iterations=1 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=12 --iterations=2 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=12 --iterations=4 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=12 --iterations=8 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=12 --iterations=16 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=16 --iterations=1 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=16 --iterations=2 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=16 --iterations=4 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=16 --iterations=8 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=16 --iterations=16 --yield=id --lists=4 >> lab2b_list.csv

	./lab2_list --threads=1 --iterations=10 --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=1 --iterations=20 --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=1 --iterations=40 --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=1 --iterations=80 --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=4 --iterations=10 --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=4 --iterations=20 --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=4 --iterations=40 --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=4 --iterations=80 --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=8 --iterations=10 --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=8 --iterations=20 --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=8 --iterations=40 --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=8 --iterations=80 --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=12 --iterations=10 --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=12 --iterations=20 --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=12 --iterations=40 --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=12 --iterations=80 --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=16 --iterations=10 --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=16 --iterations=20 --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=16 --iterations=40 --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=16 --iterations=80 --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=1 --iterations=10 --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=1 --iterations=20 --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=1 --iterations=40 --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=1 --iterations=80 --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=4 --iterations=10 --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=4 --iterations=20 --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=4 --iterations=40 --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=4 --iterations=80 --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=8 --iterations=10 --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=8 --iterations=20 --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=8 --iterations=40 --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=8 --iterations=80 --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=12 --iterations=10 --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=12 --iterations=20 --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=12 --iterations=40 --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=12 --iterations=80 --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=16 --iterations=10 --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=16 --iterations=20 --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=16 --iterations=40 --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=16 --iterations=80 --lists=4 --sync=m >> lab2b_list.csv
#for lab2b_4.png
	./lab2_list --threads=1 --iterations=1000 --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=1 --iterations=1000 --lists=8 --sync=m >> lab2b_list.csv
	./lab2_list --threads=1 --iterations=1000 --lists=16 --sync=m >> lab2b_list.csv
	./lab2_list --threads=2 --iterations=1000 --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=2 --iterations=1000 --lists=8 --sync=m >> lab2b_list.csv
	./lab2_list --threads=2 --iterations=1000 --lists=16 --sync=m >> lab2b_list.csv
	./lab2_list --threads=4 --iterations=1000 --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=4 --iterations=1000 --lists=8 --sync=m >> lab2b_list.csv
	./lab2_list --threads=4 --iterations=1000 --lists=16 --sync=m >> lab2b_list.csv
	./lab2_list --threads=8 --iterations=1000 --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=8 --iterations=1000 --lists=8 --sync=m >> lab2b_list.csv
	./lab2_list --threads=8 --iterations=1000 --lists=16 --sync=m >> lab2b_list.csv
	./lab2_list --threads=12 --iterations=1000 --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=12 --iterations=1000 --lists=8 --sync=m >> lab2b_list.csv
	./lab2_list --threads=12 --iterations=1000 --lists=16 --sync=m >> lab2b_list.csv
#For lab2b_5.png
	./lab2_list --threads=1 --iterations=1000 --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=1 --iterations=1000 --lists=8 --sync=s >> lab2b_list.csv
	./lab2_list --threads=1 --iterations=1000 --lists=16 --sync=s >> lab2b_list.csv
	./lab2_list --threads=2 --iterations=1000 --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=2 --iterations=1000 --lists=8 --sync=s >> lab2b_list.csv
	./lab2_list --threads=2 --iterations=1000 --lists=16 --sync=s >> lab2b_list.csv
	./lab2_list --threads=4 --iterations=1000 --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=4 --iterations=1000 --lists=8 --sync=s >> lab2b_list.csv
	./lab2_list --threads=4 --iterations=1000 --lists=16 --sync=s >> lab2b_list.csv
	./lab2_list --threads=8 --iterations=1000 --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=8 --iterations=1000 --lists=8 --sync=s >> lab2b_list.csv
	./lab2_list --threads=8 --iterations=1000 --lists=16 --sync=s >> lab2b_list.csv
	./lab2_list --threads=12 --iterations=1000 --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=12 --iterations=1000 --lists=8 --sync=s >> lab2b_list.csv
	./lab2_list --threads=12 --iterations=1000 --lists=16 --sync=s >> lab2b_list.csv

lab2b_list.csv: Makefile tests

#Plots the graphs
graphs: lab2b_list.csv
	gnuplot lab2b_list.gp

#Create tarball
dist: $(submission_files) Makefile profile Makefile graphs
	tar -czf lab2b-004608570.tar.gz $(submission_files) $(lab2b_list_plots) profile.out

#Make clean
.PHONY: clean
clean: 
	rm -rf lab2b-004608570.tar.gz lab2_list $(lab2b_list_plots) profile.out lab2b_list.csv 

