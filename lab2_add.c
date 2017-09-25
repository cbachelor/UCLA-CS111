/* Christopher Bachelor
** cbachelor@ucla.edu
** UCLA ID: 004608570 */

#include <stdio.h>
#include <getopt.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

/*Globals used in the program*/
int numIter;
int opt_yield;
char syncSetting;
int testSet;
pthread_mutex_t mutex;  /*Mutex for --sync=m option*/

/*The main add routine central to this program*/
void add(long long *pointer, long long value) {
    long long sum = *pointer + value;
    if (opt_yield) {
        sched_yield();
    }
    *pointer = sum;
}

/*Special case of add for the compare_and_swap atomic instruction*/
void add_compare_swap(long long *pointer, long long value) {
    long long oldVal = *pointer;
    long long sum = oldVal + value;
    do{
        oldVal = *pointer;
        sum = oldVal + value;
        if(opt_yield) {
            sched_yield();
        }
    } while (__sync_val_compare_and_swap(pointer, oldVal, sum) != oldVal);
}

/*Main function that all threads are called into*/
void* thread_func(void *pointer) {
    int i;
    /*First, make each thread add 1 to iteration times*/
    for(i = 0; i < numIter; i++) {
        /*Depending on the inputs, add synchronization options*/
        switch(syncSetting) {
            case 'm':
                pthread_mutex_lock(&mutex);
                add((long long *) pointer, 1);
                pthread_mutex_unlock(&mutex);
                break;
            case 's':
                while(__sync_lock_test_and_set(&testSet, 1) == 1) {
                    ;
                }
                add((long long *) pointer, 1);
                __sync_lock_release(&testSet);
                break;
            case 'c':
                add_compare_swap(pointer, 1);
                break;
            default:
                add((long long *)pointer, 1);
        }
    }
    /*Make each thread subtract 1 iteration times*/
    for(i = 0; i < numIter; i++) {
       switch(syncSetting) {
            case 'm':
                pthread_mutex_lock(&mutex);
                add((long long *) pointer, -1);
                pthread_mutex_unlock(&mutex);
                break;
            case 's':
                while(__sync_lock_test_and_set(&testSet, 1) == 1) {
                    ;
                }
                add((long long *) pointer, -1);
                __sync_lock_release(&testSet);
                break;
            case 'c':
                add_compare_swap(pointer, -1);
                break;
            default:
                add((long long *)pointer, -1);
        }
    }
}

/*Calculates the time it took for the threads to finish computations*/
long long calculateTime(struct timespec start_time, struct timespec end_time) {
    long long ns;
    ns = end_time.tv_sec - start_time.tv_sec;
    ns *= 1000000000;
    ns += end_time.tv_nsec;
    ns -= start_time.tv_nsec;
    return ns;
}

/*Calculates statistics of the programs and threads and prints to stdout*/
void printOutput(int numThreads, long long timeVal, long long* counter) {
    int numOps = numThreads * numIter * 2;
    long long avgOps = timeVal/((long long) numOps);
    char* setting;
    switch(syncSetting) {
        case 'm':
            if (opt_yield)
                setting = "add-yield-m";
            else
                setting = "add-m";
            break;
        case 's':
            if (opt_yield)
                setting = "add-yield-s";
            else
                setting = "add-s";
            break;
        case 'c':
            if (opt_yield)
                setting = "add-yield-c";
            else
                setting = "add-c";
            break;
        default:  
            if(opt_yield)
                setting = "add-yield-none";
            else
                setting = "add-none";
    }
    fprintf(stdout, "%s,%d,%d,%d,%lld,%lld,%lld\n", setting, numThreads, numIter, numOps,
                                                    timeVal, avgOps, *counter);
}

int main(int argc, char* argv[]) {
    int option_index = 0;
    int numThreads = 1;
    opt_yield = 0;
    numIter = 1;
    syncSetting = ' ';
    testSet = 0;
    char* sync;
    int ret = 0;
    int syncOn = 0;
    int i, rc, t;
    long long counter = 0;

    /*Properly interpret command line arguments*/
    static struct option command_arg[] =
	{
	    {"threads", optional_argument, 0, 't'},
	    {"iterations", optional_argument, 0, 'i'},
        {"yield", no_argument, 0, 'y'},
        {"sync", required_argument, 0, 's'},
	    {0, 0, 0, 0}
	};
    while(ret != -1) {
        ret = getopt_long(argc, argv, "tiys:", command_arg, &option_index);
        switch (ret) {
            case 't':
                numThreads = atoi(optarg);
                break;
            case 'i':
                numIter = atoi(optarg);
                break;
            case 'y':
                opt_yield = 1;
                break;
            case 's':
                sync = optarg;
                if(optarg != NULL)
                    syncOn = 1;
            case -1:
                break;
            default:
                fprintf(stderr, "Usage: lab2a_add [-t TOTAL_THREADS] [-i TOTAL_ITERATIONS] [-y] [-s m|s|c]\n");
                exit(1);
        }
    }
    /*Parse synchronization options*/
    if(syncOn) {
        if (sync[0] == 'm' || sync[0] == 's' || sync[0] == 'c') {
            if(sync[0] == 'm') {
                rc = pthread_mutex_init(&mutex, NULL);
                if(rc != 0) {
                    fprintf(stderr, "Error: return code from pthread_mutex_init() is %d\n", rc);
                }
            }
            syncSetting = sync[0];
        }
        else {
            fprintf(stderr, "Please specify a correct parameter to --sync\n");
            fprintf(stderr, "Usage: lab2a_add [-t TOTAL_THREADS] [-i TOTAL_ITERATIONS] [-y] [-s m|s|c]\n");
            exit(1);
        }
    }
    /*Initialize time recording variables, and start recording*/
    struct timespec start_time, end_time;
    long long ns;

    clock_gettime(CLOCK_MONOTONIC, &start_time);

    /*Initialize pthread attributes*/
    pthread_t* thread = malloc(numThreads * sizeof(pthread_t));
    pthread_attr_t attr;
    rc = pthread_attr_init(&attr);
    if (rc != 0) {
        fprintf(stderr, "Error: return code from pthread_attr_init() is %d\n", rc);
        exit(1);
    }
    rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    if (rc != 0) {
        fprintf(stderr, "Error: return code from pthread_attr_setdetachstate() is %d\n", rc);
        exit(1);
    }

    /*Initialize all threads and make them run thread_add*/
    for (i = 0; i < numThreads; i++) {
        rc = pthread_create(&thread[i], &attr, thread_func, (void *) &counter);
        if (rc != 0) {
            fprintf(stderr, "Error: return code from pthread_create() is %d\n", rc);
            exit(1);
        }
    }
    /*Wait for all the threads to terminate*/
    void* status;
    for (t = 0; t < numThreads; t++) {
        rc = pthread_join(thread[t], &status);
        if (rc != 0) {
            fprintf(stderr, "Error: return code from pthread_join() is %d\n", rc);
            exit(1);
        }
    }
    /*Capture time of end of calculations and calculate duration in milliseconds*/
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    ns = calculateTime(start_time, end_time);
    printOutput(numThreads, ns, &counter);
    free(thread);
}