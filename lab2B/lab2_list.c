// NAME: Jeffrey Chan
// EMAIL: jeffschan97@gmail.com
// ID: 004-611-638

#include<stdio.h>
#include<stdlib.h>
#include<getopt.h>
#include"SortedList.h"
#include<pthread.h>
#include<string.h>
#include<time.h>
#include<signal.h>

#define BILL 1000000000L

// global variables
int numThreads = 1;
int numIterations = 1;
int opt_yield = 0;
char syncOption;
char* yieldString = "";
pthread_mutex_t mutex;
SortedList_t* list;
SortedListElement_t* elementList;
int spinCondition = 0;
char returnString[50] = "";
long lockWaitTime = 0;

char* getTestTag() {
    // append proper yield tag
    if (strlen(yieldString) == 0) { strcat(returnString, "-none"); }
    else {
	strcat(returnString, "-");
	int k = 0;
	for (k = 0; *(yieldString + k) != '\0'; k++) {
	    if (*(yieldString + k) == 'i') { strcat(returnString, "i"); }
	    else if (*(yieldString + k) == 'd') { strcat(returnString, "d"); }
	    else if (*(yieldString + k) == 'l') { strcat(returnString, "l"); }
	}
    }

    // append proper sync tag
    if (!syncOption) { strcat(returnString, "-none"); }
    else if (syncOption == 'm') { strcat(returnString, "-m"); }
    else if (syncOption == 's') { strcat(returnString, "-s"); }

    return returnString;
}

void segFaultHandler() {
    fprintf(stderr, "error: segmentation fault\n");
    exit(2);
}

void setYieldOption(char* yieldOptions) {
    yieldString = yieldOptions;
    const char validYieldOptions[] = {'i', 'd', 'l'}; // initialize array of valid yield options

    // iterate through yield options and set opt_yield accordingly
    int k = 0;
    for (k = 0; *(yieldOptions + k) != '\0'; k++) {
        if (*(yieldOptions + k) == validYieldOptions[0]) opt_yield |= INSERT_YIELD;
        else if (*(yieldOptions + k) == validYieldOptions[1]) opt_yield |= DELETE_YIELD;
        else if (*(yieldOptions + k) == validYieldOptions[2]) opt_yield |= LOOKUP_YIELD;
        else {
            fprintf(stderr, "error: invalid yield option. valid options: [idl]\n");
            exit(1);
        }
    }
}

void generateRandomKeys(int totalElements) {
    srand(time(NULL)); // initialize random number generator

    // go through element list and insert random keys
    int k;
    for (k = 0; k < totalElements; k++) {
        int randNumber = rand() % 26; // bound rand number to an alphabet character
        char* randKey = malloc(2 * sizeof(char)); // 1 char key + null byte
        randKey[0] = 'a' + randNumber; // turn randNumber into character
        randKey[1] = '\0';

        elementList[k].key = randKey; // insert key into element
    }
}

void* listOperations(void* threadIndex) {
    int k;
    int totalElements = numThreads * numIterations;
    struct timespec lockWaitStart;
    struct timespec lockWaitEnd;

    // iterate through elements with threads and insert them into list
    for (k= *(int*)threadIndex; k < totalElements; k += numThreads) {
	switch (syncOption) {
	    case 'm':
      		if (clock_gettime(CLOCK_MONOTONIC, &lockWaitStart) == -1) { fprintf(stderr, "error: error in getting start time\n"); exit(1); }
		pthread_mutex_lock(&mutex);
    		if (clock_gettime(CLOCK_MONOTONIC, &lockWaitEnd) == -1) { fprintf(stderr, "error: error in getting stop time\n"); exit(1); }
    		lockWaitTime += BILL * (lockWaitEnd.tv_sec - lockWaitStart.tv_sec) + (lockWaitEnd.tv_nsec - lockWaitEnd.tv_nsec);
		SortedList_insert(list, &elementList[k]);
		pthread_mutex_unlock(&mutex);
		break;
	    case 's':
		while(__sync_lock_test_and_set(&spinCondition, 1));
		SortedList_insert(list, &elementList[k]);
		__sync_lock_release(&spinCondition);
		break;
	    default:
		SortedList_insert(list, &elementList[k]);
		break;
	}
    }

    switch (syncOption) {
	case 'm':
	    if (clock_gettime(CLOCK_MONOTONIC, &lockWaitStart) == -1) { fprintf(stderr, "error: error in getting start time\n"); exit(1); }
	    pthread_mutex_lock(&mutex);
	    if (clock_gettime(CLOCK_MONOTONIC, &lockWaitEnd) == -1) { fprintf(stderr, "error: error in getting stop time\n"); exit(1); }
      	    lockWaitTime += BILL * (lockWaitEnd.tv_sec - lockWaitStart.tv_sec) + (lockWaitEnd.tv_nsec - lockWaitEnd.tv_nsec);
    	    if (SortedList_length(list) == -1) {
            	fprintf(stderr, "error: failed to get length of list\n");
            	exit(2);
    	    }
	    pthread_mutex_unlock(&mutex);
	    break;
	case 's':
	    while (__sync_lock_test_and_set(&spinCondition, 1));
	    if (SortedList_length(list) == -1) {
		fprintf(stderr, "error: failed to get length of list\n");
		exit(2);
	    }
	    __sync_lock_release(&spinCondition);
	    break;
	default:
	    if (SortedList_length(list) == -1) {
		fprintf(stderr, "error: failed to get length of list\n");
		exit(2);
	    }
	    break;
    }

    // lookup and delete previously inserted elements
    SortedListElement_t *temp = NULL;
    for (k = *(int*)threadIndex; k < totalElements; k += numThreads) {
      switch (syncOption) {
        case 'm':
          if (clock_gettime(CLOCK_MONOTONIC, &lockWaitStart) == -1) { fprintf(stderr, "error: error in getting start time\n"); exit(1); }
          pthread_mutex_lock(&mutex);
          if (clock_gettime(CLOCK_MONOTONIC, &lockWaitEnd) == -1) { fprintf(stderr, "error: error in getting stop time\n"); exit(1); }
          lockWaitTime += BILL * (lockWaitEnd.tv_sec - lockWaitStart.tv_sec) + (lockWaitEnd.tv_nsec - lockWaitEnd.tv_nsec);
          temp = SortedList_lookup(list, elementList[k].key);
          if (temp == NULL) {
              fprintf(stderr, "error: failed to find element we already inserted\n");
              exit(2);
          }
          if (SortedList_delete(temp)) {
              fprintf(stderr, "error: failed to delete an element we already inserted\n");
              exit(2);
          }
          pthread_mutex_unlock(&mutex);
          break;
        case 's':
          while (__sync_lock_test_and_set(&spinCondition, 1));
          temp = SortedList_lookup(list, elementList[k].key);
          if (temp == NULL) {
              fprintf(stderr, "error: failed to find element we already inserted\n");
              exit(2);
          }
          if (SortedList_delete(temp)) {
              fprintf(stderr, "error: failed to delete an element we already inserted\n");
              exit(2);
          }
          __sync_lock_release(&spinCondition);
          break;
        default:
          temp = SortedList_lookup(list, elementList[k].key);
          if (temp == NULL) {
              fprintf(stderr, "error: failed to find element we already inserted\n");
              exit(2);
          }
          if (SortedList_delete(temp)) {
              fprintf(stderr, "error: failed to delete an element we already inserted\n");
              exit(2);
          }
	  break;
      }
    }

    return NULL;
}

int main(int argc, char **argv) {
    int option = 0; // used to hold option

    //arguments this program supports
    static struct option options[] = {
        {"threads", required_argument, 0, 't'},
        {"iterations", required_argument, 0, 'i'},
        {"yield", required_argument, 0, 'y'},
        {"sync", required_argument, 0, 's'}
    };

    // iterate through options
    while ((option = getopt_long(argc, argv, "t:i:y:s", options, NULL)) != -1) {
        switch (option) {
            case 't':
                numThreads = atoi(optarg);
                break;
            case 'i':
                numIterations = atoi(optarg);
                break;
            case 'y':
                setYieldOption(optarg);
                break;
            case 's':
                if ((optarg[0] == 'm' || optarg[0] == 's') && strlen(optarg) == 1)
                    syncOption = optarg[0];
                else {
                    fprintf(stderr, "error: unrecognized sync argument. recognized arguments: [ms]\n");
                    exit(1);
                }
		break;
            default:
                fprintf(stderr, "error: unrecognized argument\nreocgnized arguments:\n--threads=#\n--iterations=#\n--yield=[idl]\n--sync=[ms]\n");
                exit(1);
        }
    }

    // handler for segmentation faults
    signal(SIGSEGV, segFaultHandler);

    // initialize mutex if needed
    if (syncOption == 'm') { pthread_mutex_init(&mutex, NULL); }

    // initialize empty list with a head node
    list = malloc(sizeof(SortedList_t));
    list->key = NULL;
    list->next = list;
    list->prev = list;

    // create required number of list elements with random keys
    int totalElements = numThreads * numIterations;
    elementList = malloc(totalElements * sizeof(SortedListElement_t));
    generateRandomKeys(totalElements);

    // creates the number of specified threads
    pthread_t *threads = malloc(numThreads * sizeof(pthread_t));
    int* threadIds = malloc(numThreads * sizeof(int));

    // start timing
    struct timespec start;
    if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) { fprintf(stderr, "error: error in getting start time\n"); exit(1); }

    // runs the threads
    int k;
    for (k = 0; k < numThreads; k++) {
	threadIds[k] = k;
        if (pthread_create(threads + k, NULL, listOperations, &threadIds[k])) {
            fprintf(stderr, "error: error in thread creation\n");
            exit(1);
        }
    }

    // wait for all the threads to complete
    for (k = 0; k < numThreads; k++) {
        if (pthread_join(*(threads + k), NULL)) {
            fprintf(stderr, "error: error in joining threads\n");
            exit(1);
        }
    }

    // stop timing
    struct timespec end;
    if (clock_gettime(CLOCK_MONOTONIC, &end) == -1) { fprintf(stderr, "error: error in getting stop time\n"); exit(1); }

    free(elementList);
    free(threads);

    long totalTime = BILL * (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec); // calculate total time elapsed
    int numOperations = numThreads * numIterations * 3; // total operatinos performed
    long costPerOperation = totalTime / numOperations; // average cost per operation
    long averageLockWaitTime = lockWaitTime / numOperations; // average wait-for-lock

    // print test output
    char* testTag = getTestTag();
    printf("list%s,%d,%d,1,%d,%d,%d,%d\n", testTag, numThreads, numIterations, numOperations, totalTime, costPerOperation,averageLockWaitTime);

    int listLength = SortedList_length(list);
    if (listLength != 0) { exit(2); } // exit 2 if list length is not 0
    exit(0);
}
