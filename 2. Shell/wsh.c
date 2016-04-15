/*
 *  wsh3.h
 *  
 *
 *  Created by Daniel Kenefick on 3/2/11.
 *  Copyright 2011 Williams College. All rights reserved.
 *
 */


/*
 Wsh by Dan Kenefick (C) 2011
 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "cirq.c"

#define MAXARGS 100
#define MAX_PATH_LENGTH 256
#define BUILTIN_FUNCTIONS 6

#define STDIN_DUP  100
#define STDOUT_DUP 101
#define STDERR_DUP 102

extern int errno;
extern char ** environ;

void parse(char *temp, char * input, char * output);
void get_envp(char **envp1);
void get_home(char **envp1, char *i_home);

void (*id_builtin(char * command))(void *);
void bi_cd(void * newpath);
void bi_exit(void * foo);
void bi_help(void * foo);
void bi_jobs(void * foo);
void bi_kill(void * pid1);
void bi_bg (void * pid1);
void update_job_status(pid_t id, int status);
void update_jobs_list();

void execute(char * cmd, char * input, char * output);
void pipe_command(char* cmd, char * input, char * output);

//Structure to handle ctr c signals
typedef void (*sighandler_t)(int);

//args and enviornment parameters for each function
static char *argv[MAXARGS], *envp[100];


//Structures to house the built in commands: names, pointers to the functions, and a hash table
char * bi_commands[BUILTIN_FUNCTIONS] = {"cd","exit","help","jobs","kill","bg"};
void (* bi_pointers[BUILTIN_FUNCTIONS]) (void *)  = {&bi_cd,&bi_exit,&bi_help,&bi_jobs,&bi_kill,&bi_bg};

//a structure and a linked list to hold the currently running processes
typedef struct job
{
	pid_t pid;
	char * name;
	char * status;
} job;

cirq jobs;


//An external variable to hold the initial home path
char home[MAX_PATH_LENGTH];



//pipe 1 is the first of potentialy 2 open pipes, pipe2 is the second.
//pipes[] keeps track of both;

int pipe1[2];
int pipe2[2];
int * pipes[2] = {pipe1,pipe2};

//the child we are currently waiting on
pid_t child;

//flags for the current process. 
//bg indicates if we are currently doing a backgrounded job
// tst indicates the sucsess or failure of the last job
//testing indicates whether we are currently testing
// piping indicates if we are currently constructing a piping command
//append if we are appending
int bg, tst, testing, piping,append;


void handle_signal(int signo)
{
	if(signo == SIGINT){
		if(argv[0] == NULL)
			printf("\nwsh:");
		else
			printf("\n");
		fflush(stdout);
		
		if(child){
			kill(child,SIGINT);
		}
		
		return;
	}
	
	//HANDLE CHILD SIGNALING
	if(signo == SIGCHLD){
		
		
		signal(SIGCHLD, handle_signal);
		int pid, status, serrno;
		serrno = errno;
		
		while (1)
		{
			pid = waitpid (0, &status, WNOHANG | WUNTRACED);
			if ((pid == -1) | (pid == 0)){
				//perror("SIGCHLD");
				//no more child processes
				
				
				
				break;
			}else{
				if(WIFEXITED(status)){
					if(testing){
						tst = WEXITSTATUS(status);
						testing =0;
					}
				}
			}
			
			update_job_status(pid, status);
			
			
			
		}
		errno = serrno;	
		return;
	}
	
	//handle sigtstp
	if(signo == SIGTSTP){
		signal(SIGCHLD,SIG_IGN);
		fprintf(stderr,"Caught a term stop\n");
		
		if (child > 0 ){
			if(kill(child,SIGSTOP))
				perror("signal:sigtstp");
			
		}
		
		signal(SIGTSTP, handle_signal);
		signal(SIGCHLD,handle_signal);
	}
	
	
}





int main(int argc1, char *argv1[])
{
	char c;
	char *temp = (char *)malloc(sizeof(char) * 100);
	char *path = (char *)malloc(sizeof(char) * MAX_PATH_LENGTH);
	char *cmd = (char *)malloc(sizeof(char) * 100);
	char *input = (char *)malloc(sizeof(char) * MAX_PATH_LENGTH);
	char * output = (char *)malloc(sizeof(char) * MAX_PATH_LENGTH);
	piping = bg = testing = tst = append = 0;
	
	
	jobs = cq_alloc();
	
	signal(SIGINT, SIG_IGN);
	signal(SIGINT, handle_signal);
	signal(SIGCHLD, handle_signal);
	signal(SIGTSTP, handle_signal);
	
	//duplicate the essential streams for later use
	if ( !(dup2(STDIN_FILENO,STDIN_DUP)==STDIN_DUP) )
		perror("dup");
	
	if ( !(dup2(STDOUT_FILENO,STDOUT_DUP)==STDOUT_DUP) )
		perror("dup");
	
	if ( !(dup2(STDERR_FILENO,STDERR_DUP)==STDERR_DUP) )
		perror("dup");
	
	
	
	//get esssential variables from enviornemnt, copy initial home variable.
	
	get_home(environ, home);
	
	
	if(fork() == 0) {
		execvp("clear", argv1); 
		exit(1);
	} else {
		wait(NULL);
	}
	printf("wsh: ");
	fflush(stdout);
	
	
	//Read user input
	while(c != EOF) {
		
		c = getchar();
		switch(c) {
				
			case '#':
				//comment character, ignore rest of line
				while((c=getchar())){
					if (c == '\n')
						break;
				}
			case '&':
				c = getchar();
				if(c=='&'){
					testing =1;
				}else{
					bg = 1;	
				}
				
				
			case '\n':
			case ';':
				if(temp[0] == '\0') {
					//then the command was empty, and we don't need to do anything. 
					update_jobs_list();
					printf("wsh: ");
				} else {
					
					//update the jobs. This pre-emps a potential "jobs" command,
					//and double reporting of stopped processes. 
					if( c == '\n'){
						update_jobs_list();
					}
					
					//non-empty command, need to process it. 
					parse(temp,input,output);
					
					strncpy(cmd, argv[0], strlen(argv[0]));
					strncat(cmd, "\0", 1);
					
					
					execute(cmd, input,output);
					
					
					//clear the argv
					int x;
					for(x=0;argv[x]!=NULL;x++) {
						
						bzero(argv[x], strlen(argv[x])+1);
						argv[x] = NULL;
						free(argv[x]);
					}
					
					
					
					if( c == '\n'){
						printf("wsh: ");
					}
					bzero(cmd, 100);
					bzero(input,MAX_PATH_LENGTH);
					bzero(output,MAX_PATH_LENGTH);
					bg =0;
					if(piping){
						piping = 0;
						pipe1[0] =0;
						pipe1[1]=0;
						pipe2[0] = 0;
						pipe2[1] =0;
					}
				}
				bzero(temp, 100);
				break;
				
			case '|':
				//largly duplicates other cases, but with differnet functions and errors. 
				if(temp[0] == '\0') {
					update_jobs_list();
					fprintf(stderr,"Error: piping from nowhere. Check syntax and try again\n");
					printf("wsh: ");
				} else {
					
					//non-empty command, need to process it. 
					parse(temp,input,output);
					
					strncpy(cmd, argv[0], strlen(argv[0]));
					strncat(cmd, "\0", 1);
					
					pipe_command(cmd, input,output);
					
					
					//clear the argv
					int x;
					for(x=0;argv[x]!=NULL;x++) {
						
						bzero(argv[x], strlen(argv[x])+1);
						argv[x] = NULL;
						free(argv[x]);
					}
					
					bzero(cmd, 100);
					bzero(input,MAX_PATH_LENGTH);
					bzero(output,MAX_PATH_LENGTH);
					bg =0;
				}
				bzero(temp, 100);
				break;
				
				
			default:
				//save the char, and move on.
				strncat(temp, &c, 1);
				break;
		}
	}
	
	free(cmd);
	free(temp);
	free(path);
	free(input);
	free(output);
	
	bi_exit(NULL);
	return 0;
}


//a function to get the initial home out of th env. strings and keep it where we can use it
void get_home(char **envp1, char *i_home)
{
	int x = 0;
	char *tmp;
	while(1) {
		tmp = strstr(envp1[x], "HOME");
		if(tmp == NULL) {
			x++;
		} else {
			break;
		}
	}
	tmp = strstr(tmp, "/");
	
	if(strlen(tmp)>MAX_PATH_LENGTH){
		fprintf(stderr,"Home Path is too long, cannot save");
		return;
	}
	
	strncpy(i_home, tmp, strlen(tmp));
}

void parse(char *temp,char *input,char *output )
{
	char *tmp = temp;
	int size = 0;
	char segment[100];
	bzero(segment, 100);
	char * dest = argv[size];
	
	
	while(*tmp != '\0') {
		//clumbsy - replace with a linked list?
		//at the very least provide a built in function to change the number. 
		if(size == MAXARGS){
			fprintf(stderr, "Too many arguments. Current Maximum of %d \n", MAXARGS);
			break;
		}
		
		
		switch (*tmp) {
				
			case '<':
				
				
				//we dont want to countwhitepace as args
				if(!(*segment == '\0')){
					
					
					
					
					
					if(dest == NULL){
						if(dest == argv[size]){
							dest = (char *)malloc(sizeof(char) * strlen(segment) + 1);
							argv[size] = dest;
						}
					}
					else {
						bzero(dest, strlen(dest));
					}
					
					//copy the segment into the args 
					strncpy(dest, segment, strlen(segment));
					strncat(dest, "\0", 1);
					bzero(segment, 100);
					
					if(dest == argv[size]){
						size++;
					}
					
				}
				
				dest = input;
				break;
				
			case'>':
				
				if(*(tmp+1)=='>'){
					append = O_APPEND;
					tmp++;
				}
				
				
				
				//we dont want to countwhitepace as args
				if(!(*segment == '\0')){
					
					
					
					
					
					if(dest == NULL){
						if(dest == argv[size]){
							dest = (char *)malloc(sizeof(char) * strlen(segment) + 1);
							argv[size] = dest;
						}
					}
					else {
						bzero(dest, strlen(dest));
					}
					
					//copy the segment into the args 
					strncpy(dest, segment, strlen(segment));
					strncat(dest, "\0", 1);
					bzero(segment, 100);
					
					if(dest == argv[size]){
						size++;
					}
					
				}
				
				dest = output;
				break;
				
			case ' ':
				
				//we dont want to countwhitepace as args
				if(!(*segment == '\0')){
					
					
					
					
					
					if(dest == NULL){
						if(dest == argv[size]){
							dest = (char *)malloc(sizeof(char) * strlen(segment) + 1);
							argv[size] = dest;
							bzero(dest,strlen(segment) + 1);
						}
					}
					else {
						bzero(dest, strlen(dest));
					}
					
					//copy the segment into the args 
					strncpy(dest, segment, strlen(segment));
					strncat(dest, "", 1);
					bzero(segment, 100);
					
					if(dest == argv[size]){
						size++;
					}
					dest = argv[size];
					
				}
				
				
				break;
				
			default:
				strncat(segment, tmp, 1);
				
				
		}	
		
		
		tmp++;
	}
	
	//get the last argument, possibly not terminated by whitespace.
	if(!(*segment == '\0')){
		
		if(dest == NULL){
			if(dest == argv[size]){
				dest = (char *)malloc(sizeof(char) * strlen(segment) + 1);
				argv[size] = dest;
				bzero(dest,strlen(segment) + 1);
			}
		}
		else {
			bzero(dest, strlen(dest));
		}
		
		strncpy(dest, segment, strlen(segment));
		strncat(dest, "", 1);
	}
}

void (*id_builtin(char * command))(void *)
{
	void (* pointer)(void *) = NULL;
	int index=0;
	
	
	for(;index<BUILTIN_FUNCTIONS;index++){
		if(!strcmp(command,bi_commands[index])){
			pointer = bi_pointers[index];
			break;
		}
	}
	return pointer;
	
}

void bi_help(void * foo){
	
	
	if (foo != NULL){
		char * arg = foo;
		if(!strcmp(arg,"-Me")){
			fprintf(stderr,"There is no helping you\n");
			return;
		}
		
		fprintf(stderr,"Error: no arguments allowed for help command\n");
		return;
	}
	
	fprintf(stdout,"A list of built in commands:\n\nhelp : this command\n\ncd (path) : change the current working directory. path specifies the path to change to. No arguments will change to your home directory.\n\njobs : list the currently executing jobs and their job number \n\nkill [-a] (job number) : kill a specified job. The -a flag kills all jobs.\n\nexit : exit wsh \n bg: background a stopped process \n");
	
}

void bi_jobs(void * foo){
	
	if (foo != NULL){
		fprintf(stderr,"Error: no arguments allowed for jobs command\n");
		return;
	}
	
	int size = cq_size(jobs);
	job * j;
	
	if (size == 0){
		return;
	}
	
	for(;size>0;size--){
		j = cq_peek(jobs);
		fprintf(stdout,"[%d]   %s   %s\n", j->pid, j->name, j->status);
		
		cq_rot(jobs);
	}
	return;
}

void bi_kill(void * pid1)
{
	
	int size = cq_size(jobs);
	job * j;
	
	char * pid = (char *)pid1;
	
	
	
	
	if(pid == NULL){
		fprintf(stderr, "Error: no Job number. Please specify a job number to kill\n");
		return;
	}
	
	
	
	if(!strcmp(pid,"-a")){
		
		
		if(argv[1]){
			for(;size>0;size--){
				j =cq_deq(jobs);
				if( kill(j->pid, SIGKILL) ){
					perror("kill");
					return;
				}
			}	
		}
		
		return;
	}
	
	int n;
	char * name;
	
	n = atoi(pid);
	
	if (n == 0 ){
		fprintf(stderr, " error: invalid job number: %s. please specify an integer job\n",pid);
		return;
	}
	
	pid_t id = (pid_t)n;
	
	
	
	
	
	
	name =  NULL;
	
	for(;size>0;size--){
		j =cq_peek(jobs);
		if (j->pid == id){
			name = j->name;
			break;
		}
		cq_rot(jobs);
	}
	
	if (name == NULL){
		fprintf(stderr, "Job not found in current list of jobs\n");
		return;
	}
	
	if( kill(n, SIGTERM) ){
		perror("kill");
		return;
	}
	
	return;
} 

void bi_exit(void * foo){
	if (foo != NULL){
		fprintf(stderr,"Error: no arguments allowed for exit command\n");
		return;
	}
	
	//kill all child processes
	int size = cq_size(jobs);
	job * j;
	
	for(;size>0;size--){
		j =cq_deq(jobs);
		if( kill(j->pid, SIGKILL) ){
			perror("kill");
			return;
		}
	}
	
	cq_free(jobs);
	
	int i;
	for(i=0;envp[i]!=NULL;i++)
		free(envp[i]);
	
	exit(EXIT_SUCCESS);
	
}


void bi_cd(void * newpath){
	char * npath = (char *)newpath;
	//an empty operation should be interpreted as returning home
	if((npath== NULL)){
		npath = "~";
	}
	
	char * thome = malloc( sizeof(char) * (strlen(home)+strlen((char *)npath) )+1 );
	strcpy(thome,home);
	
	if(*npath == '~'){
		npath++;
		npath = strcat( thome,npath);
	}
	
	if( (chdir(npath))){
		perror("cd");
	}
	
	free(thome);
}


void bi_bg (void * pid1){
	
	int size = cq_size(jobs);
	
	if (size == 0){
		fprintf(stderr, "Error: no Jobs.\n");
		return;
	}
	
	
	job * j;
	
	char * pid = (char *)pid1;
	
	
	if(pid == NULL){
		fprintf(stderr, "Error: no Job number. Please specify a job number to kill\n");
		return;
	}
	
	int n;
	char * name;
	
	n = atoi(pid);
	
	if (n == 0 ){
		fprintf(stderr, " error: invalid job number: %s. please specify an integer job\n",pid);
		return;
	}
	
	pid_t id = (pid_t)n;
	
	
	name =  NULL;
	
	for(;size>0;size--){
		j =cq_peek(jobs);
		if (j->pid == id){
			name = j->name;
			break;
		}
		cq_rot(jobs);
	}
	
	if (name == NULL){
		fprintf(stderr, "Job not found in current list of jobs\n");
		return;
	}
	
	if( strcmp("Stopped",j->status)){
		fprintf(stderr, "Job is not stopped\n");
		return;
		
	}
	
	//let the job continue
	if( kill(n, SIGCONT) ){
		perror("kill");
		return;
	}
	
	char * stat = "Running";
	free(j->status);
	j-> status = malloc( sizeof(char) * strlen(stat) +1 );
	strcpy(j->status,stat);
	
}






void execute(char * cmd, char * input, char * output){
	
	//first we handle input. If non null, open the input file
	//open the apropriate file, redirect stdin to this files stream,
	//execute the command, and change stdinput back. 
	
	//fprintf(stderr,"the pipe is, at execute, %d %d \n",pipes[0][0],pipes[0][1]);
	
	//open the input, handle errors.
	
	if(tst){
		tst = 0;
		return;
	}
	
	
	
	//detemine if this is a built in comand. if so, execute it. 
	//	
	void (*pointer) (void *) = NULL;
	if((pointer = id_builtin(cmd)) ){
		
		
		if( !(*input == '\0') ){
			
			if (piping){
				fprintf(stderr, "Error, cannot redirect input while piping to a command");
				return;
			}
			
			int infid;
			if(  (infid = open(input, O_RDONLY )) < 0){
				perror("open:in");
				return;
			}
			
			if(dup2(infid,STDIN_FILENO) < 0){				
				perror("open:in:dup");				
				if((close(infid)<0 )){
					perror("open:in:dup:error:close");
				}				
				return;
			}
			
			if((close(infid)<0 )){
				perror("open:in:close");
				return;
			}
			
			
		}
		
		//open the output, handle errors
		if( !(*output == '\0') ){
			int outfid;
			
			if(  ((outfid = open(output, O_WRONLY | O_CREAT| append, S_IRUSR | S_IWUSR |S_IRGRP |S_IWGRP| S_IROTH  )) < 0) ){
				perror("open:out1");
				append = 0;
				return;
			}			
			
			if(dup2(outfid,STDOUT_FILENO) < 0){				
				perror("open:out:dup");				
				if((close(outfid)<0 )){
					perror("open:out:dup:error:close");
					append = 0;
					return;
				}
				append = 0;
				return;
			}
			
			if((close(outfid)<0 )){
				perror("open:out:close");
				return;
			}						
		}
		
		
		(*pointer) (argv[1]);
		
	} else {
		
		
		
		
		
		// not a built in command, need to interpret and execute it.
		if((child = fork()) == 0){
			
			if( !(*input == '\0') ){
				
				if (piping){
					fprintf(stderr, "Error, cannor redirect input while piping to a command");
					return;
				}
				
				int infid;
				if(  (infid = open(input, O_RDONLY )) < 0){
					perror("open:in");
					return;
				}
				
				if(dup2(infid,STDIN_FILENO) < 0){				
					perror("open:in:dup");				
					if((close(infid)<0 )){
						perror("open:in:dup:error:close");
					}				
					return;
				}
				
				if((close(infid)<0 )){
					perror("open:in:close");
					return;
				}
				
				
				
			}
			
			//open the output, handle errors
			if( !(*output == '\0') ){
				int outfid;
				
				if(  ((outfid = open(output, O_WRONLY | O_CREAT| append, S_IRUSR | S_IWUSR |S_IRGRP |S_IWGRP| S_IROTH  )) < 0) ){
					perror("open:out1");
					append = 0;
					return;
				}			
				
				if(dup2(outfid,STDOUT_FILENO) < 0){				
					perror("open:out:dup");				
					if((close(outfid)<0 )){
						perror("open:out:dup:error:close");
						append = 0;
						return;
					}
					append = 0;
					return;
				}
				
				if((close(outfid)<0 )){
					perror("open:out:close");
					return;
				}						
			}
			
			signal(SIGTSTP,SIG_IGN);
			
			if(bg){
				//ignore certain keyboard signals
				//signal(SIGTERM,SIG_IGN);
				signal(SIGINT,SIG_IGN);
				
			}
			
			if( execvp(cmd,argv) ){
				perror(cmd);
				
				//Do I need to free memory here? 
				_exit (EXIT_FAILURE);
			}
			
			
		} else{
			//If we want run it in the background...
			
			
			
			
			if(bg){
				fprintf(stdout,"[%d]\n",child);
				
				
				//a new job struct to keep track
				job * j = malloc(sizeof(job));
				if (j == NULL){
					fprintf(stderr,"Could not allocate job data type in execute\n");
				}
				j->pid = child;
				
				j->name = malloc( sizeof(char) * strlen(cmd)+1);
				j->status = malloc ( sizeof(char) * strlen("Running")+1 );
				strcpy(j->name,cmd);
				strcpy(j->status,"Running");
				
				cq_enq(jobs,j);
				
			}
			//If we want to wait for it to finish...
			else{
				
				int status;
				waitpid(child,&status,WUNTRACED);	
				
				
				if(WIFSTOPPED(status)) {
					//child  stopped
					
					//a new job struct to keep track
					job * j = malloc(sizeof(job));
					if (j == NULL){
						fprintf(stderr,"Could not allocate job data type in execute\n");
					}
					j->pid = child;
					
					j->name = malloc( sizeof(char) * strlen(cmd)+1);
					j->status = malloc ( sizeof(char) * strlen("Running")+1 );
					strcpy(j->name,cmd);
					strcpy(j->status,"Stopped");
					
					cq_enq(jobs,j);
					
					
					fprintf(stdout,"[%d]   %s   %s\n",child,cmd,"Stopped");
					
				}
			}
			child = 0;
			if(piping){
				
				close(pipes[0][0]);				
			}	
			
		}
		
	}
	
	//put stdin back
	if(!(*input == '\0') | piping ){	
		close(STDIN_FILENO);
		if(dup2(STDIN_DUP,STDIN_FILENO) < 0){
			perror("open:in:dupback");
		}			
	}
	
	//put stdout back
	if( !(*output == '\0') | piping ){	
		close(STDOUT_FILENO);
		if(dup2(STDOUT_DUP,STDOUT_FILENO) < 0){
			perror("open:out:dupback");
		}			
	}
	
	
}


void update_job_status(pid_t id, int status){
	
	
	
	int size = cq_size(jobs);
	
	if(size == 0)
		return;
	
	job * j;
	char * stat;
	
	if( WIFEXITED(status) ){
		stat = "Done";
	}
	if (WIFSIGNALED(status)) {
		stat = "Terminated";
	}
	
	
	for(;size>0;size--){
		j =cq_peek(jobs);
		
		if (  j->pid == id ){
			break;
		}
		
		
		cq_rot(jobs);
	}
	if (size !=0){
		free(j->status);
		j-> status = malloc( sizeof(char) * strlen(stat) +1 );
		strcpy(j->status,stat);
	}
	
	
}

//to be called after every newline, to annouce terminated / finished jobs and 
//remove them from the list. 
void update_jobs_list(){
	int size = cq_size(jobs);
	
	if(size == 0)
		return;
	
	job * j;
	
	for(;size>0;size--){
		j =cq_peek(jobs);
		
		if (!strcmp( j->status, "Terminated") | !strcmp( j->status, "Done")){
			
			fprintf(stdout,"[%d]   %s   %s\n",j->pid,j->name,j->status);
			
			free(j->name);
			free(j->status);
			free(j);
			cq_deq(jobs);
			continue;
		}
		cq_rot(jobs);
	}
	return;
}

void pipe_command(char* cmd, char * input, char * output){
	
	
	//create a new pipe, not overwriting if we already have one
	
	
	pipe(pipes[piping]);	//my favorite line of code 
	

	
	if((child = fork())){
		//parent process
		
		
		//close write end of pipe
		if(close(pipes[piping][1])<0){
			perror("pipe:in:close");
			_exit (EXIT_FAILURE);
		} 
		
		//redirect the read end
		if(dup2(pipes[piping][0],STDIN_FILENO) < 0){				
			perror("pipe:in:dup");	
			
			
			if((close(pipes[piping][0])<0 )){
				perror("pipe:in:dup:error:close");
			}				
			_exit (EXIT_FAILURE);
		}
		
		//close the read end of the pipe.
		if( close(pipes[piping][0]) <0 ){
			perror("end pipe in close");
		}
		
		
		
		
		if(!piping){
			piping++;
		} else {
			//close the original pipe, no loner needed
			close(pipes[0][0]);
			close(pipes[0][1]);			
		}
		
		//go ont to interpret other command
		return;
		
		
	} else{
		
		//child process
		
		if(strcmp(output,"")){
			fprintf(stderr,"Error in pipe: redirected output not allowed");
			_exit (EXIT_FAILURE);
		}
		
		if(piping && strcmp(input,"")){
			fprintf(stderr,"Error in pipe: redirected input not allowed");
			_exit (EXIT_FAILURE);
		}
		
		
		
		
		//close the read end of the pipe we jsut opened
		if(close(pipes[piping][0])<0){
			perror("pipe:in:close");
			_exit (EXIT_FAILURE);
		}  
		
		
		// redirect std out to this pipe
		if(dup2(pipes[piping][1],STDOUT_FILENO) < 0){				
			perror("pipe:out:dup");				
			if((close(pipes[piping][1])<0 )){
				perror("pipe:out:dup:error:close");
			}				
			_exit (EXIT_FAILURE);
		}
		
		//close the write end
		if (close(pipes[piping][1])< 0){
			perror("pipe:out:close");	
		}
		
		
		execute(cmd,input,output);
		
		
		
		exit( EXIT_SUCCESS);
	}
	
	
	
	
}

/* A Haiku
 *Coding, like rowing-
 *is highly repetative-
 *makes you hate machines-
 */



