/*
 * A program to play "hot potato" with n threads.
 * (c) 2011 duane a. bailey
 *
 * This program spawns n tasks and passes m potatoes around between the
 * participants.  Each potato is retransmitted a certain number of times
 * (an epoch), makes a final in-order round, and is then shelved.
 * Tasks finish when they participate in the final round for all m potatoes.
 */
#include <stdio.h>
#include <stdlib.h>
#include "ts.h"

int potatoCount = 10;
int ntasks = 4;
int epoch = 100;
int logging = 0;

int worker(int *taskID)
{
	/* get my task "address"; potatoes addressed to me have this address */
	int myID = *(int*)taskID;
	int offset = 1;
	int done = 0;
	int countdown = potatoCount;
	/* initial potatoes are "hot"; as they retire, the cool off */
	if (logging) printf("Worker %d has begun.\n",myID);
	
	do {
		int age,potatoID,dest;
		if (inp("s?i?ii","hot potato",&potatoID,&age,myID)) {
			if (logging) printf("task %d received hot potato %d, age %d.\n",
								myID,potatoID,age);
			age++;
			dest = (myID + offset)%ntasks;
			offset++;
			if (offset == ntasks) offset = 1;
			if (age == epoch) {
				out("sii","cool potato",potatoID,0);
				if (logging) printf("task %d passed along potato %d, age %d, to %d for final round\n",
									myID,potatoID,age,dest);
			} else {
				out("siii","hot potato",potatoID,age,dest);
				if (logging) printf("task %d passed along hot potato %d, age %d, to %d\n",
									myID,potatoID,age,dest);
			}
		} else
			if (inp("s?ii","cool potato",&potatoID,myID)) {
				if (logging) printf("task %d received cool potato %d in round\n",
									myID,potatoID);
				done = 0 == --countdown;
				if (myID == ntasks-1) {
					
					
					
					
					out("si","cold potato",potatoID);
					if (logging) printf("task %d shelved cold potato %d\n",
										myID,potatoID);
				} else {
					out("sii","cool potato",potatoID,myID+1);
					if (logging) printf("task %d passed cool potato %d to %d in final round\n",
										myID,potatoID,dest);
				}
			}
	} while (!done);
	if (logging) printf("Task %d finishing up.\n",myID);
	return myID;
}

void Usage(char *progName)
{
	fprintf(stderr,"Usage: %s [-d] [-t taskCount] [-p potatoCount] [-l lifetime]\n",progName);
	fprintf(stderr,"\t-t\tspecify task-parallelism [currently %d]\n",ntasks);
	fprintf(stderr,"\t-p\tpotato count [currently %d]\n",potatoCount);
	fprintf(stderr,"\t-l\tpotato lifetime [currently %d]\n",epoch);
	fprintf(stderr,"\t-d\tdebug messages [currently %s]\n",
			logging?"on":"off");
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
						ntasks = atoi(*++argv);
						argc--;
						break;
					case 'p':
						potatoCount = atoi(*++argv);
						argc--;
						break;
					case 'l':
						epoch = atoi(*++argv);
						argc--;
						break;
					case 'd':
						logging=1;
						break;
					case 'h':
						Usage(progName);
						break;
				}
			}
		}
	}
}

int main(int argc, char **argv)
{
	int i,*task;
	parseArgs(argc,argv);
	task = (int*)malloc(ntasks*sizeof(int));
	
	/* mash-up the potatoes: */
	for (i = 0; i < potatoCount; i++) {
		int dest = (i*13)%ntasks;
		out("siii","hot potato",i,0,dest);
		if (logging) printf("Created potato %d for task %d.\n",
							i,dest);
	}
	
	/* create tasks threads */
	for (i = 0; i < ntasks; i++) {
		task[i] = i;
		if (logging) printf("Creating task %d.\n",i);
		eval("s!i","done",worker,&task[i]);
	}
	
	/* 
	 * wait for tasks threads to complete;
	 * order is unimportant
	 */
	printf("Tasks completed: "); fflush(stdout);
	for (i = 0; i < ntasks; i++) {
		int id;
		in("s?i","done",&id);
		printf(" %d",id);
	}
	putchar('\n');
	
	/*
	 * call in the potatoes
	 */  
	for (i = 0; i < potatoCount; i++) {
		int j;
		in("s?i","cold potato",&j);
		printf("Potato %d survived.\n",j);
	}
	printf("%d tasks and %d potatoes accounted for.\n",ntasks,potatoCount);
	return 0;
}