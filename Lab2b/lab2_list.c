/* Christopher Bachelor
** cbachelor@ucla.edu
** UCLA ID: 004608570
** FOR LAB 2B */

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
int numLists = 1;    /*Number of lists with --lists, default 1*/
int* testSet;       /*Array of integers for test-and-set operation for each list*/
char syncopts = ' ';
int opt_yield = 0;
SortedList_t* list; /*List we use*/
SortedListElement_t* elements;
int* elementsListIndex; /*Array whose index corresponds to elements, holds value of which list to insert*/
pthread_mutex_t* mutex;
unsigned long long* threadTime; /*Array used to store lock acquisition time per thread*/

/*Signal handler to catch SEGFAULTS*/
void signal_handler(int signum) {
  fprintf(stderr, "Error: Segfault caught\n");
   exit(2);
}

/*Calculates the time it took for the threads to finish computations*/
unsigned long long calculateTime(struct timespec start_time, struct timespec end_time) {
    unsigned long long ns; 
    ns = end_time.tv_sec - start_time.tv_sec;
    ns *= 1000000000;
    ns += end_time.tv_nsec;
    ns -= start_time.tv_nsec;
    return ns;
}

/*Prints out the desired output after parsing through the given options*/
void printOutput(unsigned long long timeVal ) {
    int i;
    unsigned long long lockTime = 0;
    /*Calculation for total number of mutex operations:
        number of elements * (insert + lookup/delete) + each thread * lock sublist mutex */
    int numMutex = numThreads * numIter * 2 + numThreads * numLists;
    int numOps = numThreads * numIter * 3;
    int printNone = 0;
    /*Calculate total lock acquisition time*/
    if(syncopts == 'm' || syncopts == 's') {    
        for(i = 0; i < numThreads; i++) {
            lockTime += threadTime[i];
        }
        lockTime = lockTime / numMutex;
    }
    unsigned long long avgOps = timeVal/((unsigned long long) numOps);
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
    fprintf(stdout, ",%d,%d,%d,%d,%lld,%lld,%llu\n", numThreads, numIter, numLists, 
                                                    numOps, timeVal, avgOps, lockTime);
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
        if (syncTemp == 'm' || syncTemp == 's') {
            syncopts = syncTemp;
        }
        else {
            fprintf(stderr, "Please specify a correct parameter to --sync\n");
            fprintf(stderr, "Usage: lab2a_add [-t TOTAL_THREADS] [-i TOTAL_ITERATIONS]\n");
            exit(1);
        }
}

/*Initializes all the list elements with a random key
  Additionally hashes each key with the sub-list it 
  belongs to in a separate array, but with the same index*/
void keyGen(int numElements) {
    int i, t, randomLetter;
    int sum = 0;
    srand(time(NULL));
    /*The key is based on KEY_SIZE random alphabet characters (initially 10)*/
    for (i = 0; i < numElements; i++) {
        char* tempKey = malloc((KEY_SIZE+1) * sizeof(char));
        if(tempKey == NULL) {
            fprintf(stderr, "Error: Failed to allocate key elements\n");
            exit(1);
        }
        for(t = 0; t < KEY_SIZE; t++) {
            randomLetter = rand() % 26;
            sum += randomLetter;
            tempKey[t] = randomLetter + 'a';
        }
        tempKey[KEY_SIZE] = '\0';
        /*Detached elements have a random key and its pointers are NULL*/
        elements[i].key = tempKey;
        elements[i].next = NULL;
        elements[i].prev = NULL;
        elementsListIndex[i] = sum % numLists;
        sum = 0;
    }
}

/*Initialize numLists length of lists, each pointing to itself*/
void initializeList() {
    int i, rc;
    list = malloc(sizeof(SortedList_t) * numLists);
    if (list == NULL) {
        fprintf(stderr, "Error: failed to allocate list\n");
        exit(1);
    }
    for (i = 0; i < numLists; i++) {
        list[i].key = NULL;
        list[i].next = &list[i];
        list[i].prev = &list[i];
    }
    /*Initialize an array of mutexes used per list*/
    if(syncopts== 'm') {
        mutex = malloc(sizeof(pthread_mutex_t) * numLists);
        if (mutex == NULL) {
            fprintf(stderr, "Error: failed to allocate list\n");
            exit(1);
        }
        for (i = 0; i < numLists; i++) {
            rc = pthread_mutex_init(&mutex[i], NULL);
            if(rc != 0) {
                fprintf(stderr, "Error: return code from pthread_mutex_init() is %d\n", rc);
            }
        }
    }
    /*Initialize an array of test-and-set variables used for spin lock */
    else if (syncopts == 's') {
        testSet = malloc(sizeof(int) * numLists);
        if (testSet == NULL) {
            fprintf(stderr, "Error: failed to allocate list\n");
            exit(1);
        }
        for(i = 0; i < numLists; i++) {
            testSet[i] = 0;
        }
    }
}


/*Main function that each thread calls*/
void* thread_func(void* tidPtr) {
    int tid = *((int *) tidPtr);
    int i, t, ret, sumTemp;
    int sum = 0;
    SortedListElement_t* elementPtr;
    /*Variables used to calculate lock acquisition time*/
    struct timespec start_time, end_time;
    /*Each thread gets its own chunk of elements based on their thread ID*/
    for(i = tid * numIter; i < tid * numIter + numIter; i++) {
       /*First insert all elements into the list*/
        switch(syncopts) {
            /*Different options for synchronization depending on input parameters*/
            case 'm':
                /*Calculate the time it takes to get a hold of the mutex*/
                clock_gettime(CLOCK_MONOTONIC, &start_time);
                pthread_mutex_lock(&mutex[elementsListIndex[i]]);
                clock_gettime(CLOCK_MONOTONIC, &end_time);
                threadTime[tid] += calculateTime(start_time, end_time);
                SortedList_insert(&list[elementsListIndex[i]], &elements[i]);
                pthread_mutex_unlock(&mutex[elementsListIndex[i]]);
                break;
            case 's':
                /*Calculate the time it takes to get a hold of the spin lock*/
                clock_gettime(CLOCK_MONOTONIC, &start_time);
                while(__sync_lock_test_and_set(&testSet[elementsListIndex[i]], 1) == 1) {
                    ;
                }
                clock_gettime(CLOCK_MONOTONIC, &end_time);
                threadTime[tid] += calculateTime(start_time, end_time);
                SortedList_insert(&list[elementsListIndex[i]], &elements[i]);
                __sync_lock_release(&testSet[elementsListIndex[i]]);
                break;
            default:
                SortedList_insert(&list[elementsListIndex[i]], &elements[i]);
        }
    }

    /*Each thread performs SortedList_length calculation*/
    switch(syncopts) {
        case 'm':
            for(t = 0; t < numLists; t++) {
                clock_gettime(CLOCK_MONOTONIC, &start_time);
                pthread_mutex_lock(&mutex[elementsListIndex[t]]);
                clock_gettime(CLOCK_MONOTONIC, &end_time);
                threadTime[tid] += calculateTime(start_time, end_time);
                sumTemp = SortedList_length(&list[elementsListIndex[t]]);
                if(sumTemp == -1) {
                    fprintf(stderr, "Error: Corrupted list in SortedList_length()");
                    exit(2);
                }
                pthread_mutex_unlock(&mutex[elementsListIndex[t]]);
                sum += sumTemp;
            }
            break;
        case 's':
            for(t = 0; t < numLists; t++) {
                clock_gettime(CLOCK_MONOTONIC, &start_time);
                while(__sync_lock_test_and_set(&testSet[elementsListIndex[t]], 1) == 1) {
                        ;
                }
                clock_gettime(CLOCK_MONOTONIC, &end_time);
                threadTime[tid] += calculateTime(start_time, end_time);
                sumTemp += SortedList_length(&list[elementsListIndex[t]]);
                if(sumTemp == -1) {
                    fprintf(stderr, "Error: Corrupted list in SortedList_length()");
                    exit(2);
                }
                __sync_lock_release(&testSet[elementsListIndex[t]]);
                sum += sumTemp;
            }
            break;
        default:
            for (t = 0; t < numLists; t++) {
                sumTemp += SortedList_length(&list[elementsListIndex[i]]);
                if(sumTemp == -1) {
                    fprintf(stderr, "Error: Corrupted list in SortedList_length()");
                    exit(2);
                }
                sum += sumTemp;
            }
    }

    /*Each thread looks up the elements they inserted and deletes them, with given synchronization options*/
    for(i = tid * numIter; i < tid * numIter + numIter; i++) {
        switch (syncopts) {
            case 'm':
                clock_gettime(CLOCK_MONOTONIC, &start_time);
                pthread_mutex_lock(&mutex[elementsListIndex[i]]);
                clock_gettime(CLOCK_MONOTONIC, &end_time);
                threadTime[tid] += calculateTime(start_time, end_time);
                elementPtr = SortedList_lookup(&list[elementsListIndex[i]], elements[i].key);
                if(elementPtr == NULL) {
                    fprintf(stderr, "Error: Couldn't find element\n");
                    exit(2);
                }
                ret = SortedList_delete(elementPtr);
                if (ret == 1) {
                    fprintf(stderr, "Error: Corrupted prev/next pointers in SortedList_delete()\n");
                    exit(2);
                }
                pthread_mutex_unlock(&mutex[elementsListIndex[i]]);
                break;
            case 's':
                clock_gettime(CLOCK_MONOTONIC, &start_time);
                while(__sync_lock_test_and_set(&testSet[elementsListIndex[i]], 1) == 1) {
                    ;
                }
                clock_gettime(CLOCK_MONOTONIC, &end_time);
                threadTime[tid] += calculateTime(start_time, end_time);
                elementPtr = SortedList_lookup(&list[elementsListIndex[i]], elements[i].key);
                if(elementPtr == NULL) {
                    fprintf(stderr, "Error: Couldn't find element\n");
                    exit(2);
                }
                ret = SortedList_delete(elementPtr);
                if (ret == 1) {
                    fprintf(stderr, "Error: Corrupted prev/next pointers in SortedList_delete()\n");
                    exit(2);
                }
                __sync_lock_release(&testSet[elementsListIndex[i]]);
                break;
            default:
                elementPtr = SortedList_lookup(&list[elementsListIndex[i]], elements[i].key);
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
    char* syncTemp;

    /*Properly interpret command line arguments*/
    static struct option command_arg[] =
	{
	    {"threads", optional_argument, 0, 't'},
	    {"iterations", optional_argument, 0, 'i'},
        {"yield", required_argument, 0, 'y'},
        {"sync", required_argument, 0, 's'},
        {"lists", optional_argument, 0, 'l'}, 
	    {0, 0, 0, 0}
	};
    while(ret != -1) {
        ret = getopt_long(argc, argv, "tiy:s:l", command_arg, &option_index);
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
            case 'l':
                numLists = atoi(optarg);
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

    initializeList();
    
    /*Allocate memory for the elements, and create additional array that 
        holds value of the list it belongs to*/
    elements = malloc(numElements * (sizeof(SortedListElement_t)));
    elementsListIndex = malloc(numElements  * (sizeof(int)));
    if(elements == NULL || elementsListIndex == NULL) {
        fprintf(stderr, "Error: Failed to allocate elements\n");
        exit(1);
    }
    keyGen(numElements);
    
    /*Allocate space to store lock acquisition time per thread*/
    threadTime = malloc(sizeof(unsigned long long) * numThreads);
    for (i = 0; i < numThreads; i++) {
        threadTime[i] = 0;
    }
    /*Initialize time recording variables, and start recording*/
    struct timespec start_time, end_time;
    unsigned long long ns;
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
    /*Final list calculation: If not 0, then synchronization error*/
    int sum = 0;
    int sumTemp;
    for (i = 0; i < numLists; i++) {
        sumTemp = SortedList_length(&list[i]);
        if(sumTemp == -1) {
            fprintf(stderr, "Error: List length is not 0 after threads have terminated\n");
            exit(2);
        }
        sum += sumTemp;
    }

    printOutput(ns);

    /*Free all memory that was allocated*/
    for(i = 0; i < numIter*numThreads; i++) {
        free((void *) elements[i].key);     
    }
    free(elements);
    free(tid);
    free(thread);
    free(threadTime);
    if(syncopts == 'm') {
       for(i = 0; i < numLists; i++) {
            pthread_mutex_destroy(&mutex[i]);
       }
       free(mutex);
    }
    free(elementsListIndex);
    free(list);
    if(syncopts == 's') {
        free((void *)testSet);
    }
}