/*
 * A simple tuple-space based CPU exerciser.
 * (c) 2011 duane a. bailey
 *
 * This program, when linked with a tuple-space, counts the number of
 * primes from 2 through some maximum value.  The approach is to simply
 * test each value for factors (there are faster ways, but we're looking
 * to occupy the machine).
 *
 * A worker task checks out a range or "chunk" of integers to be tested and
 * counts the number that are prime.  The process continues until all the
 * integers less than a maximum value have been considered.
 *
 * When one thread is used, the worker considers all the value.
 * When more than one thread is used, the chunking causes the load to be
 * balanaced.
 *
 * By default, the program makes use of one worker task.  
 *
 * Optionally, you can throw the -n switch ("not really") which progresses
 * without actually performing the computation.  It may be useful for computing
 * the amount of overhead in the program.
 */
#include <stdio.h>
#include <stdlib.h>
#include "ts.h"

int chunkSize = 1000;
int tasks = 1;
int maxValue = 1000000;
int compute = 1;

void Usage(char *progName)
{
	fprintf(stderr,"Usage: %s [-n] [-t taskCount] [-c chunkSize] [-m maxValue]\n",progName);
	fprintf(stderr,"\t-t\tspecify task-parallelism [currently %d]\n",tasks);
	fprintf(stderr,"\t-c\tchunk size [currently %d]\n",chunkSize);
	fprintf(stderr,"\t-m\tmaximum value [currently %d]\n",maxValue);
	fprintf(stderr,"\t-n\tdon't compute [currently %scomputing]\n",compute?"":"not ");
	exit(1);
}

void parseArgs(int argc, char **argv)
{
	char *progName = *argv;
	while (argv++, --argc) {
		char *arg = *argv;
		if (*arg == '-') {
			while (*++arg) {
				switch (*arg) {
					default:
						fprintf(stderr,"I'm confused: -%c\n",*arg);
						Usage(progName);
						break;
					case 't':
						tasks = atoi(*++argv);
						argc--;
						break;
					case 'c':
						chunkSize = atoi(*++argv);
						argc--;
						break;
					case 'm':
						maxValue = atoi(*++argv);
						argc--;
						break;
					case 'n':
						compute = 0;
						break;
					case 'h':
						Usage(progName);
						break;
				}
			}
		}
	}
}

/*
 * Check to see if n is prime.
 */
int isPrime(int n)
{
	int f;
	if (!compute) return 0;
	for (f = 2; f*f <= n; f += 1+(f&1)) {
		if (n%f == 0) return 0;
	}
	return 1;
}

/*
 * This worker runs, counting primes in chunks, until no more chunks
 * remain.  At the end, it reports the number of primes enountered,
 * through the tuple-space.
 *
 * Repeatedly:
 *  consumes: ("N", base value)
 *  generates: ("N", base value + chunk)
 * At end:
 *  generates: ("task primes", task id, count)
 *             ("done", task id)
 */ 
int worker(void *taskId)
{
	int i = *(int*)taskId;
	int current, barrier, count = 0;
	
	do {
		/* grab a chunk of computation */
		in("s?i","N",&current);
		barrier = current+chunkSize;
		if (barrier > maxValue) barrier = maxValue;
		out("si","N",barrier);
		/* count the number of primes in this chunk */
		while (current < barrier) {
			if (isPrime(current)) {
				count++;
			}
			current += 1 + (current&1);
		}
	} while (current < maxValue);
	out("sii","task primes",i,count);
	return i;
}

int main(int argc, char **argv)
{
	int i,id,count,number,*task;
	parseArgs(argc,argv);
	task = (int*)malloc(tasks*sizeof(int));
	
	/* "prime" the pump: */
	out("si","N",2);
	
	/* create tasks threads */
	for (i = 0; i < tasks; i++) {
		task[i] = i;
		eval("s!i","done",worker,&task[i]);
	}
	
	/* 
	 * wait for tasks threads to complete;
	 * order is unimportant
	 */
	for (i = 0; i < tasks; i++) {
		in("s?i","done",&id);
	}
	
	/*
	 * total up all the counts
	 */  
	count = 0;
	for (i = 0; i < tasks; i++) {
		rd("si?i","task primes",i,&number);
		count += number;
	}
	printf("There are %d primes below %d.\n",count,maxValue);
	return 0;
}