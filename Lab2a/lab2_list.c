/* Christopher Bachelor
** cbachelor@ucla.edu
** UCLA ID: 004608570 */

#include <stdio.h>
#include <getopt.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include "SortedList.h"
#include <time.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>

/*Globals used in the program*/
#define KEY_SIZE 10
int numThreads = 1; /*Number of threads, default 1*/
int numIter = 1;    /*Number of iterations, default 1*/
int testSet;
char syncopts;
int opt_yield = 0;
SortedList_t* list; /*List we use*/
SortedListElement_t* elements;
pthread_mutex_t mutex;

/*Signal handler to catch SEGFAULTS*/
void signal_handler(int signum) {
  fprintf(stderr, "Error: Segfault caught\n");
   exit(2);
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

/*Prints out the desired output after parsing through the given options*/
void printOutput(long long timeVal ) {
    int numOps = numThreads * numIter * 3;
    int numLists = 1;
    int printNone = 0;
    long long avgOps = timeVal/((long long) numOps);
    fprintf(stdout, "list-");
    if(opt_yield & INSERT_YIELD) {
         fprintf(stdout, "i");
         printNone = 1;
    }
    if( opt_yield & DELETE_YIELD) {
         fprintf(stdout, "d");
         printNone = 1;
    }
    if (opt_yield & LOOKUP_YIELD) {
         fprintf(stdout, "l");
         printNone = 1;
    }
    if(!printNone) {
         fprintf(stdout, "none");
    }
    fprintf(stdout, "-");
    if (syncopts != 's' && syncopts != 'm') {
        fprintf(stdout, "none");
    }
    else {
         fprintf(stdout, "%c", syncopts);
    }
    fprintf(stdout, ",%d,%d,%d,%d,%lld,%lld\n", numThreads, numIter, numLists, 
                                                            numOps, timeVal, avgOps);
}

/*Initializes the opt_yield bits based on the options given in --yield*/
void yieldSet(char* yieldParam) {
    int i = 0;
    while(yieldParam[i] != '\0') {
        if(yieldParam[i]== 'i' ) {
            opt_yield |= INSERT_YIELD;
        }
        else if (yieldParam[i] == 'd') {
            opt_yield |= DELETE_YIELD;
        }
        else if (yieldParam[i] == 'l') {
            opt_yield |= LOOKUP_YIELD;
        }
        else {
            fprintf(stderr, "Please specify a correct parameter to --yield\n");
            fprintf(stderr, "Usage: lab2a_list [-t TOTAL_THREADS] [-i TOTAL_ITERATIONS]\n");
            exit(1);
        }
        i++;
    }
}

/*Initializes the sync parameters given by --sync*/
void syncSet (char syncTemp){
    int rc;
        if (syncTemp == 'm' || syncTemp == 's') {
            if(syncTemp == 'm') {
                rc = pthread_mutex_init(&mutex, NULL);
                if(rc != 0) {
                    fprintf(stderr, "Error: return code from pthread_mutex_init() is %d\n", rc);
                }
            }
            syncopts = syncTemp;
        }
        else {
            fprintf(stderr, "Please specify a correct parameter to --sync\n");
            fprintf(stderr, "Usage: lab2a_add [-t TOTAL_THREADS] [-i TOTAL_ITERATIONS]\n");
            exit(1);
        }
}

/*Initializes all the list elements with a random key*/
void keyGen(int numElements) {
    int i, t, thread, threadTen, threadOne, randomLetter;
    srand(time(NULL));
    /*The key begins with the number of the thread(modded to 2 digits) that holds the element
    followed by KEY_SIZE random characters (initially 8)*/
    for (i = 0; i < numElements; i++) {
        thread = i % numThreads;
        threadTen = thread/10;
        threadOne = thread % 10;
        char* tempKey = malloc((KEY_SIZE+1) * sizeof(char));
        if(tempKey == NULL) {
            fprintf(stderr, "Error: Failed to allocate key elements\n");
            exit(1);
        }
        tempKey[0] = threadTen + '0'; /*Converts int to ASCII digit*/
        tempKey[1] = threadOne + '0';
        for(t = 2; t < KEY_SIZE; t++) {
            randomLetter = rand() % 26;
            tempKey[t] = randomLetter + 'a';
        }
        tempKey[KEY_SIZE] = '\0';
        /*Detached elements have a random key and its pointers are NULL*/
        elements[i].key = tempKey;
        elements[i].next = NULL;
        elements[i].prev = NULL;
    }
}

/*Main function that each thread calls*/
void* thread_func(void* tidPtr) {
    int tid = *((int *) tidPtr);
    int i, ret;
    SortedListElement_t* elementPtr;

    /*Each thread gets its own chunk of elements based on their thread ID*/
    for(i = tid * numIter; i < tid * numIter + numIter; i++) {
       /*First insert all elements into the list*/
        switch(syncopts) {
            /*Different options for synchronization depending on input parameters*/
            case 'm':
                pthread_mutex_lock(&mutex);
                SortedList_insert(list, &elements[i]);
                pthread_mutex_unlock(&mutex);
                break;
            case 's':
                while(__sync_lock_test_and_set(&testSet, 1) == 1) {
                    ;
                }
                SortedList_insert(list, &elements[i]);
                __sync_lock_release(&testSet);
                break;
            default:
                SortedList_insert(list, &elements[i]);
        }
    }
    /*Each thread performs SortedList_length calculation*/
    switch(syncopts) {
        case 'm':
            pthread_mutex_lock(&mutex);
            ret = SortedList_length(list);
            if(ret == -1) {
                fprintf(stderr, "Error: Corrupted list in SortedList_length()");
                exit(2);
            }
            pthread_mutex_unlock(&mutex);
            break;
        case 's':
             while(__sync_lock_test_and_set(&testSet, 1) == 1) {
                    ;
            }
            ret = SortedList_length(list);
            if(ret == -1) {
                fprintf(stderr, "Error: Corrupted list in SortedList_length()");
                exit(2);
            }
            __sync_lock_release(&testSet);
            break;
        default:
            ret = SortedList_length(list);
            if(ret == -1) {
                fprintf(stderr, "Error: Corrupted list in SortedList_length()");
                exit(2);
            }
    }
    /*Each thread looks up the elements they inserted and deletes them, with given synchronization options*/
    for(i = tid * numIter; i < tid * numIter + numIter; i++) {
        switch (syncopts) {
            case 'm':
                pthread_mutex_lock(&mutex);
                elementPtr = SortedList_lookup(list, elements[i].key);
                if(elementPtr == NULL) {
                    fprintf(stderr, "Error: Couldn't find element\n");
                    exit(2);
                }
                ret = SortedList_delete(elementPtr);
                if (ret == 1) {
                    fprintf(stderr, "Error: Corrupted prev/next pointers in SortedList_delete()\n");
                    exit(2);
                }
                pthread_mutex_unlock(&mutex);
                break;
            case 's':
                while(__sync_lock_test_and_set(&testSet, 1) == 1) {
                    ;
                }
                elementPtr = SortedList_lookup(list, elements[i].key);
                if(elementPtr == NULL) {
                    fprintf(stderr, "Error: Couldn't find element\n");
                    exit(2);
                }
                ret = SortedList_delete(elementPtr);
                if (ret == 1) {
                    fprintf(stderr, "Error: Corrupted prev/next pointers in SortedList_delete()\n");
                    exit(2);
                }
                __sync_lock_release(&testSet);
                break;
            default:
                elementPtr = SortedList_lookup(list, elements[i].key);
                if(elementPtr == NULL) {
                    fprintf(stderr, "Error: Couldn't find element\n");
                    exit(2);
                }
               // fprintf(stderr, "Deleting %s\n", elements[i].key);
                ret = SortedList_delete(elementPtr);
                if (ret == 1) {
                    fprintf(stderr, "Error: Corrupted prev/next pointers in SortedList_delete()\n");
                    exit(2);
                }
        }
    }

}

int main(int argc, char* argv[]) {
    int option_index = 0;
    int ret = 0;
    int yieldOn = 0;
    int syncOn = 0;
    char* yieldTemp;
    int rc, i, t;
    testSet = 0;
    syncopts  = ' ';
    char* syncTemp;

    /*Properly interpret command line arguments*/
    static struct option command_arg[] =
	{
	    {"threads", optional_argument, 0, 't'},
	    {"iterations", optional_argument, 0, 'i'},
        {"yield", required_argument, 0, 'y'},
        {"sync", required_argument, 0, 's'},
	    {0, 0, 0, 0}
	};
    while(ret != -1) {
        ret = getopt_long(argc, argv, "tiy:s:", command_arg, &option_index);
        switch (ret) {
            case 't':
                numThreads = atoi(optarg);
                break;
            case 'i':
                numIter = atoi(optarg);
                break;
            case 'y':
                yieldTemp = optarg;
                yieldOn = 1;
                break;
            case 's':
                syncTemp = optarg;
                if(optarg != NULL)
                    syncOn = 1;
                break;
            case -1:
                break;
            default:
                fprintf(stderr, "Usage: lab2a_list [-t TOTAL_THREADS] [-i TOTAL_ITERATIONS] [-y i|d|l] [-s m|s]\n");
                exit(1);
        }
    }
    /*Interpret yield options and sync options given*/
    if(yieldOn) {
        yieldSet(yieldTemp);
    }
    if(syncOn) {
        syncSet(syncTemp[0]);
    }

    /*Start signal handling in case of SEGFAULT*/
    signal(SIGSEGV, signal_handler);

    int numElements = numThreads * numIter;
    list = malloc(sizeof(SortedList_t));
    list->key = NULL;
    list->next = list;
    list->prev = list;
    
    /*Allocate memory for the list*/
    elements = malloc(numElements*(sizeof(SortedListElement_t)));
    if(elements == NULL) {
        fprintf(stderr, "Error: Failed to allocate elements\n");
        exit(1);
    }
    keyGen(numElements);
    
    /*Initialize time recording variables, and start recording*/
    struct timespec start_time, end_time;
    long long ns;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    /*Initialize pthread attributes*/
    pthread_t* thread = malloc(numThreads * sizeof(pthread_t));
    int* tid = malloc(numThreads * sizeof(int));
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

    /*Initialize all threads and make them run the thread functions*/
    for (i = 0; i < numThreads; i++) {
        tid[i] = i;
        rc = pthread_create(&thread[i], &attr, thread_func, (void *) &tid[i]);
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
    rc = SortedList_length(list);
    if(rc == -1) {
        fprintf(stderr, "Error: List length is not 0 after threads have terminated\n");
        exit(2);
    }

    printOutput(ns);

    /*Free all memory that was allocated*/
    for(i = 0; i < numIter*numThreads; i++) {
        free((void *) elements[i].key);     
    }
    free(elements);
    free(list);
    free(tid);
    free(thread);
}