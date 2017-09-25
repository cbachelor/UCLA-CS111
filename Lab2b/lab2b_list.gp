#! /usr/bin/gnuplot
#
# Christopher Bachelor
# UCLA ID: 004608570
# cbachelor@ucla.edu
#
# purpose: 
# 	 generate data reduction graphs for lab2b
#
# input: lab2b_list.csv
#	1. test name
#	2. # threads
#	3. # iterations per thread
#	4. # lists 
#	5. total # of operations
#	6. total run time
#	7. average time per operation
#   8. average wait-for-lock
# output: 
# 	lab2b_1.png ... throughput vs number of threads for mutex and spin-lock synchronized list oprations.
#	lab2b_2.png ... mean time per mutex wait and mean time per operation for mutex-synchronized list operations.
#	lab2b_3.png ... successful iterations vs threads for each synchronization method.
#	lab2b_4.png ... throughput vs number of threads for mutex synchronized partitioned lists.
#	lab2b_5.png ... throughput vs number of threads for spin-lock-synchronized partitioned lists.
#

# general plot parameters
set terminal png
set datafile separator ","

# how many threads/iterations we can run without failure (w/o yielding)
set title "List-2b-1: Number of Operations per Second vs Threads, no yield"
set xlabel "Threads" 
set ylabel "Throughput (operations/sec)"
set logscale y 
set output 'lab2b_1.png'

# grep out synchonized list mutex-protected, non-yield results
plot \
     "< grep 'list-none-m,[1-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($6)) \
	title 'mutex synchronized' with linespoints lc rgb 'red', \
     "< grep 'list-none-s,[1-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($6)) \
	title 'spin lock synchronized' with linespoints lc rgb 'green'


# Average wait-for-mutex time vs threads
set title "List-2b-2: Wait-for-Lock Time and Average Time per Operation"
set xlabel "Threads"
set ylabel "Average time (ns)"
set logscale y
set output 'lab2b_2.png'

# grep out synchonized list mutex-protected, non-yield results
plot \
     "< grep 'list-none-m,[1-9][46]*,1000,1,' lab2b_list.csv" using ($2):($8) \
	title 'wait-for-mutex-time' with linespoints lc rgb 'red', \
     "< grep 'list-none-s,[1-9][46]*,1000,1,' lab2b_list.csv" using ($2):($7) \
	title 'average time per operation' with linespoints lc rgb 'green'


# For plot 3
set title "List-2b-3: Unprotected Threads and Iterations that run without failure, yield=id"
set xlabel "Threads"
set xrange [0.5:16.5]
set ylabel "Successful Iterations"
set logscale y 
set output 'lab2b_3.png'

plot \
     "< grep 'list-id-none,[1-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
	title 'Without protection' with points lc rgb 'green' pt 7, \
     "< grep 'list-none-m,[1-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
	title 'With protection sync=m' with points lc rgb 'red' pt 9, \
     "< grep 'list-none-s,[1-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
	title 'With protection sync=m' with points lc rgb 'blue' pt 4 


# For plot 4
set title "List-2b-4: Operations per Second vs Threads, Mutex Protected"
set xlabel "Threads"
set logscale x  2
set xrange [0.5:32]
set ylabel "Throughput (operations/sec)"
set logscale y 10
set output 'lab2b_4.png'

plot \
     "< grep 'list-none-m,[1-9][2]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($6)) \
	title 'List Size = 1' with linespoints lc rgb 'red', \
    "< grep 'list-none-m,[1-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($6)) \
	title 'List Size = 4' with linespoints lc rgb 'blue', \
	"< grep 'list-none-m,[1-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($6)) \
	title 'List Size = 8' with linespoints lc rgb 'green', \
	"< grep 'list-none-m,[1-9]*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($6)) \
	title 'List Size = 16' with linespoints lc rgb 'orange'


# For plot 5
set title "List-2b-5: Operations per Second vs Threads, Spin-lock Protected"
set xlabel "Threads"
set logscale x 2
set xrange [0.5:32]
set ylabel "Throughput (operations/sec)"
set logscale y 10
set output 'lab2b_5.png'

plot \
     "< grep 'list-none-s,[1-9][2]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($6)) \
	title 'List Size = 1' with linespoints lc rgb 'red', \
    "< grep 'list-none-s,[1-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($6)) \
	title 'List Size = 4' with linespoints lc rgb 'blue', \
	"< grep 'list-none-s,[1-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($6)) \
	title 'List Size = 8' with linespoints lc rgb 'green', \
	"< grep 'list-none-s,[1-9]*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($6)) \
	title 'List Size = 16' with linespoints lc rgb 'orange'
	