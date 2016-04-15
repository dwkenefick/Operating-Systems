#include <string.h>
#include "hash.c"
#include <stdarg.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#define MAXSIZE 1000
#define MAX_TUPLE_LENGTH 10;

int out(char* form,...);
int inp(char *form,...);
int rdp (char * form, ...);
int eval (char* form, ...);
void evalh (void * args[2]);
int in (char * form,...);
int rd (char * form,...);
void init();
void test1();
void test2();
char * repeat(char * string);

hash h;
pthread_mutex_t write_mutex, read_mutex;
pthread_cond_t change = PTHREAD_COND_INITIALIZER;

pthread_once_t once_control = PTHREAD_ONCE_INIT;

typedef union{
	char * s;
	int i;

} data;
/*
int main(){
	
	 //eval test
	int len;
	eval("!s!i",repeat,"length",strlen,"length");
	sleep(1);
	
	inp("s?i","length",&len);
	printf("%d",len);
	
	return 0;
	 
	
	//mutex test
	pthread_t one;
	pthread_t two;
	void* x;
	pthread_create(&one,NULL,test1,x);
	pthread_create(&two,NULL,test2,x);
	pthread_join(one, NULL);

	
	return 0;
}
*/
void add(data ** dat, char* key);

int out(char * form,...){
	pthread_once(&once_control, init);
	va_list ap;
	va_start(ap,form);
	
	int len = strlen(form);
	if(len == 0){
		fprintf(stderr,"out: invalid format, must have non-zero lenght");
		return -1;
	}

	int x, count;
	char c;
	
	int tempi;
	char* segment;
	data ** dat = malloc(sizeof(data*) *len) ;
	
	count = 0;
	char * form1 = malloc(sizeof(char)*len);
	
	for(x=0;x<len;x++){
		
		
		switch (c = *form) {
			case 'i':
				tempi = va_arg(ap, int);

				dat[count] = malloc(sizeof(data));
				dat[count]->i = tempi;
				count++;
				form1[x] = c;
				
				break;
				
			case 's':
				segment = va_arg(ap, char *);							
				dat[count] = malloc(sizeof(data));
				dat[count]->s = malloc(sizeof(char)* strlen(segment) +1);
				dat[count]->s = strcpy(dat[count]->s,segment);
				count++;
				form1[x] = c;
				break;
				
				
				
			case'!':
				//get the next char to determine the return type
				len--;
				form++;
				form1[x] = *form;
				
				if (form1[x] == 'i'){
					int (* funct) (void *) = va_arg(ap, void * );
					
					void * arg = va_arg(ap,void *);
					
					int result = funct(arg);
					
					dat[x] = malloc(sizeof (data));
					dat[x]->i = result;
					
					
				} else if(form1[x] =='s'){
					char * (* funct) (void *) = va_arg(ap, void * );
					
					void * arg = va_arg(ap,void *);
					
					char * result = funct(arg);
					
					dat[x] = malloc(sizeof (data));
					dat[x]->s = malloc(sizeof(char)* strlen(result) +1);
					dat[x]->s = strcpy(dat[x]->s,result);
					
					
				} else{
					fprintf(stderr,"invalid format");
					return -1;
				}
				
				
				break;
				
				
				
				
			default:
				fprintf(stderr,"out in ts:invalid format string");
				return -1;
				
				break;
		}
		
		
		form++;
	}
	form1 = realloc(form1,sizeof(char)*len+1);
	form1[x] = '\0';
	va_end(ap);
	
	/***************Critical section *****************/
	
	pthread_mutex_lock(&write_mutex);
	
	add(dat,form1);
	
	
	pthread_cond_broadcast(&change);
	
	pthread_mutex_unlock(&write_mutex);

	
	/**************end critical section***************/
	
	return 0;		
}

/*
RW
 */
int inp (char *form,...){
	pthread_once(&once_control, init);
	char c;
	
	int tempi,x;
	//char* tempc;
	char* segment;
	
	char* formc = form;
	
	int len = strlen(form);
	char* form1 = malloc( sizeof (char) * len +1);
	
	
	data ** dat= malloc(sizeof(data *) *len);
	//data * tempd;
	
	//an array to hold the pointers that we may need to use;
	void * pointers[len];
	
	
	va_list ap;
	va_start(ap, form);
	
	//iterate through: construct the "no ?" string, construct the data array. 
	for(x=0;x<len;x++){
		
		
		switch (c = *form) {
			case 'i':
			  tempi = va_arg(ap, int);
				
				//tempd = malloc(sizeof (data));
				//tempd->i = tempi;
				dat[x] = malloc(sizeof (data));
				dat[x]->i = tempi;
				form1[x] = c;
				
				break;
				
			case 's':
				segment = va_arg(ap, char*);
				//tempc = malloc(sizeof(char)* strlen(segment) +1);
				//tempc = strcpy(tempc,segment);
				
				//tempd = malloc(sizeof (data));
				//tempd->s = tempc;
				
				
				dat[x] = malloc(sizeof (data));
				dat[x]->s = malloc(sizeof(char)* strlen(segment) +1);
				dat[x]->s = strcpy(dat[x]->s,segment);
			
				form1[x] = c;
				
				break;
				
				
			case '?':
				pointers[x] = va_arg(ap, void*);
				dat[x] = NULL;
				len--;
												
				form++;
				
				
				form1[x] = *form;
				
				
				break;
				
				
			default:
				fprintf(stderr,"inp:invalid format string");
				return -1;
				
				break;
		}
		
		form++;
	}
	
	form1 = realloc(form1,sizeof(char)*len+1);
	form1[x] = '\0';
	
	//Need to get a mutex here, to avoid multiple rotating, etc.
	
	/***************Critical section *****************/
	
	pthread_mutex_lock(&write_mutex);
	
	cirq * q = ht_get(h, form1);

	
	if(!q){
	  free(form1);
	  //get rid of dat
	  for (x=0;x<len;x++){
	    if(form1[x] =='s' && dat[x]){
	      free(dat[x]->s);
	       free(dat[x]);
	    }
	    //free(dat[x]);
	  }
	  free(dat);
		pthread_mutex_unlock(&write_mutex);
		return 0;
	}
	
	//search the queue for a matching array of data items.
	
	int size = cq_size(*q);
	int result = 0;
	data ** temp;
	
	for(;size>0;size--){
		temp = cq_peek(*q);
		
		//compare the two
		
		for(x=0;x<len; x++){
			
			if ( (data *)(dat[x]) != NULL){
				switch (c =form1[x]) {
					case 'i':
						result += dat[x]->i - temp[x]->i;
						break;
					case 's':
						result+= strcmp((dat[x])->s ,(temp[x])->s);					
						break;
					default:
						fprintf(stderr,"error");
						pthread_mutex_unlock(&write_mutex);
						return 0;
						break;
				}
				
			}
			if(result)
				break;
		}						
		
		
		if( !result ){
			
			//get rid of dat
			for (x=0;x<len;x++){
				if(form1[x] =='s' && dat[x]){
					free(dat[x]->s);
					//free(dat[x]);
				}
				free(dat[x]);
			}
				
				free(dat);	
			
			dat = cq_deq(*q);
			result = 1;
			break;
		}
		
		
		cq_rot(*q);	
		result = 0;
		
	}
	//if we found a match, we need to copy the pointers coressponding to the ?'s 
	if(result){
		len = strlen(formc);
		
		
		for(x = 0;x<len;x++){
			
			if (formc[x] == '?'){
				formc++;
				
				switch (c = formc[x]) {
					case 'i':
						*(int *)(pointers[x]) = dat[x]->i;
						
						break;						
					case 's':	
						strcpy(pointers[x],dat[x]->s);
						
						break;
					default:
						fprintf(stderr,"inp: invalid format");
						pthread_mutex_unlock(&write_mutex);
						return 0;
						break;
				}
			}
			
			
		}
	}
	
	//get rid of dat
	len = strlen(form1);
	for (x=0;x<len;x++){
	  if(form1[x] =='s' && (dat[x]!=NULL)){
		  free(dat[x]->s);
		  
		  
		}
	  free(dat[x]);
	}
	
	
	pthread_mutex_unlock(&write_mutex);
	
	
	/**************end critical section***************/
	
	free(dat);	
	free(form1);
	va_end(ap);
	
	return result;
			
}

//Read only
int rdp (char *form, ...){
	pthread_once(&once_control, init);
	char c;
	
	int tempi,x;
	char* tempc;
	char* segment;
	
	char* formc = form;
	
	int len = strlen(form);
	char* form1 = malloc( sizeof (char) * len +1);
	
	
	data ** dat= malloc(sizeof(data *) *len);
	data * tempd;
	
	//an array to hold the pointers that we may need to use;
	void * pointers[len];
	
	
	va_list ap;
	va_start(ap, form);
	
	//iterate through: construct the "no ?" string, construct the data array. 
	for(x=0;x<len;x++){
		
		
		switch (c = *form) {
			case 'i':
				tempi = va_arg(ap, int);
				
				tempd = malloc(sizeof (data));
				tempd->i = tempi;
				dat[x] = tempd;
				
				form1[x] = c;
				
				break;
				
			case 's':
				segment = va_arg(ap, char*);
				tempc = malloc(sizeof(char)* strlen(segment) +1);
				tempc = strcpy(tempc,segment);
				
				tempd = malloc(sizeof (data));
				tempd->s = tempc;
				
				
				dat[x] = tempd;
				
				form1[x] = c;
				
				break;
				
				
			case '?':
				pointers[x] = va_arg(ap, void*);
				dat[x] = NULL;
				len--;
				
				form++;
				
				
				form1[x] = *form;
				
				
				break;
				
			default:
				fprintf(stderr,"inp:invalid format string");
				return -1;
				
				break;
		}
		
		form++;
	}
	
	cirq * q = ht_get(h, form1);
	
	//have to wait here, for matching form 
	if(!q){
	  free(form1);
	  //get rid of dat
	  for (x=0;x<len;x++){
	    if(form1[x] =='s' && dat[x]){
	      free(dat[x]->s);
	      free(dat[x]);
	    }
	  }
	  free(dat);
	    return 0;
	}
	
	//search the queue for a matching array of data items.
	//also might have to do this loop multiple times
	int size = cq_size(*q);
	int result = 0;
	data ** temp;
	
	
	/******************* Critical section *****************/
	
	pthread_mutex_lock(&write_mutex);
	
	for(;size>0;size--){
		temp = cq_peek(*q);
		
		//compare the two
		
		for(x=0;x<len; x++){
			
			if ( (data *)(dat[x]) != NULL){
				switch (c =form1[x]) {
					case 'i':
						result += dat[x]->i - temp[x]->i;
						break;
					case 's':
						result+= strcmp((dat[x])->s ,(temp[x])->s);					
						break;
					default:
						fprintf(stderr,"error");
						return 0;
						break;
				}
				
			}
			if(result)
				break;
		}						
		
		
		if( !result ){
			
			//get rid of dat
			for (x=0;x<len;x++){
				if(form1[x] =='s'){
					free(dat[x]->s);
					free(dat[x]);
				}
			}
			
			free(dat);	
			
			dat = cq_peek(*q);
			result = 1;
			break;
		}
		
		
		cq_rot(*q);	
		result = 0;
		
	}
	
	pthread_mutex_unlock(&write_mutex);
	/****************** end crititcal section ************************/
	
	//if we found a match, we need to copy the pointers coressponding to the ?'s 
	if(result){
		len = strlen(formc);
		for(x = 0;x<len;x++){
			if (*formc == '?'){
				formc++;
				
				switch (c = *formc) {
					case 'i':
						*(int *)(pointers[x]) = dat[x]->i;
						
						break;						
					case 'c':	
						strcpy(pointers[x],dat[x]->s);
						
						break;
					default:
						fprintf(stderr,"rdp:invalid format");
						return 0;
						break;
				}								
			}
			formc++;
		}
	}
	
	//get rid of dat
	len = strlen(form1);
	for (x=0;x<len;x++){
		if(form1[x] =='s'){
			free(dat[x]->s);
			free(dat[x]);
		}
	       
	}
	
	free(dat);	
	free(form1);
	va_end(ap);
	
	return result;
	
}

//eval returns immidately, spinning a thread to do the work.
//things to remember: thread safety 

int eval (char * form,...){
	
	//copy the arguments and then pass them to the helper function
	pthread_once(&once_control, init);
	int len = strlen(form);
	void ** args = malloc(sizeof(void *) * (1+len));
	
	char * formc = malloc(sizeof(char)*strlen(form)+1);
	strcpy (formc,form);

	args[0] = (void *) formc;

	va_list ap;
	va_start(ap,form);
	
	//copy the arguments
	
	int x;
	char c;
	char * segment;
	for(x=1;x<=len;x++){
		
		
		switch (c = form[x-1]) {
			case 'i':
				args[x] = malloc(sizeof(int));
				*(int *)args[x] = va_arg(ap,int);
				
				break;
				
			case 's':
				segment = va_arg(ap, char *);
				
				args[x] = malloc(sizeof(char)*strlen(segment)+1);
				strcpy((char *)args[x],segment);
				break;
				
			case'!':

			
				args[x] = va_arg(ap,void *);
				x++;
				
				args[x] = va_arg(ap,void *);
				
				break;
				
				
				
				
			default:
				fprintf(stderr,"out in ts:invalid format string");
				return -1;
				
				break;
		}
	}	
	
	
	
	
	

	
	va_end(ap);
	
	pthread_t tid;
	
	if(pthread_create(&tid,NULL,evalh,args)){
		fprintf(stderr,"thread error");
		return -1;
	}
	//pthread_join( tid, NULL);
	
	return 0;
}

//make sure to free the string and args
void evalh(void ** args){
	
	
	char * form = args[0];
	
	int len = strlen(form);
	if(len == 0){
		fprintf(stderr,"out: invalid format, must have non-zero lenght");
		return;
	}

	int x, count;
	char c;
	
	int tempi;
	char* segment;
	data ** dat = malloc(sizeof(data*) *len) ;
	
	count = 0;
	char * form1 = malloc(sizeof(char)*len);
	
	for(x=0;x<len;x++){
		
		
		switch (c = *form) {
			case 'i':
				tempi = *(int *)args[x+1];
				
				dat[count] = malloc(sizeof(data));
				dat[count]->i = tempi;
				count++;
				form1[x] = c;
				
				break;
				
			case 's':
				segment = args[x+1];
						
				dat[count] = malloc(sizeof(data));
				dat[count]->s = malloc(sizeof(char)* strlen(segment) +1);
				dat[count]->s = strcpy(dat[count]->s,segment);
				count++;
				form1[x] = c;
				break;
				
			case'!':
				//get the next char to determine the return type
				len--;
				form++;
				form1[x] = *form;
				
				if (form1[x] == 'i'){
					int (* funct) (void *) = args[x+1];
					
					void * arg = args[x+2];
					
					int result = funct(arg);
					
					dat[x] = malloc(sizeof (data));
					dat[x]->i = result;
					
					
				} else if(form1[x] =='s'){
					char * (* funct) (void *) = args[x+1];
					
					void * arg = args[x+2];
					
					char * result = funct(arg);
					
					dat[x] = malloc(sizeof (data));
					dat[x]->s = malloc(sizeof(char)* strlen(result) +1);
					dat[x]->s = strcpy(dat[x]->s,result);
					
					
				} else{
					fprintf(stderr,"invalid format");
					return;
				}
				
				
				break;
				
				
				
				
			default:
				fprintf(stderr,"out in ts:invalid format string");
				return;
				
				break;
		}
		
		
		form++;
	}
	/***************Critical section *****************/
	
	pthread_mutex_lock(&write_mutex);
	
	add(dat,form1);
	
	pthread_cond_broadcast(&change);
	
	pthread_mutex_unlock(&write_mutex);
	
	
	/**************end critical section***************/
	free(args[0]);
	free(args[1]);
	free(args);
	free(form1);
	return;		
}

//add the tuple to the CQ of matching 
void add(data ** dat, char* key){
	cirq* q = ht_get(h, key);	
	if(!q){
		cirq *temp = malloc(sizeof(cirq));
		*temp= cq_alloc();
		q = temp;
		cq_enq(*q, dat);
		ht_put(h,key,q);
	} else{
		cq_enq(*q, dat);		
	}
	
	return;	
}

int in (char * form, ...){
	pthread_once(&once_control, init);
	char c;
	
	int tempi,x;
	//char* tempc;
	char* segment;
	
	char* formc = form;
	
	int len = strlen(form);
	char* form1 = malloc( sizeof (char) * len +1);
	
	
	data ** dat= malloc(sizeof(data *) *len);
	//data * tempd;
	
	//an array to hold the pointers that we may need to use;
	void * pointers[len];
	
	
	va_list ap;
	va_start(ap, form);
	
	//iterate through: construct the "no ?" string, construct the data array. 
	for(x=0;x<len;x++){
		
		
		switch (c = *form) {
			case 'i':
				tempi = va_arg(ap, int);
				
				//tempd = malloc(sizeof (data));
				//tempd->i = tempi;
				dat[x] = malloc(sizeof (data));
				dat[x]->i = tempi;
				form1[x] = c;
				
				break;
				
			case 's':
				segment = va_arg(ap, char*);
				//tempc = malloc(sizeof(char)* strlen(segment) +1);
				//tempc = strcpy(tempc,segment);
				
				//tempd = malloc(sizeof (data));
				//tempd->s = tempc;
				
				
				dat[x] = malloc(sizeof (data));
				dat[x]->s = malloc(sizeof(char)* strlen(segment) +1);
				dat[x]->s = strcpy(dat[x]->s,segment);
				
				form1[x] = c;
				
				break;
				
				
			case '?':
				pointers[x] = va_arg(ap, void*);
				dat[x] = NULL;
				len--;
				
				form++;
				
				
				form1[x] = *form;
				
				
				break;
				
				
			default:
				fprintf(stderr,"inp:invalid format string");
				return -1;
				
				break;
		}
		
		form++;
	}
	
	form1 = realloc(form1,sizeof(char)*len+1);
	form1[x] = '\0';
	int result = 0;
	
	pthread_mutex_lock(&write_mutex);
	/***************Critical section *****************/
	do{
		
		
		cirq * q = ht_get(h, form1);
		int result = 0;
		
		if(q){
			
			//search the queue for a matching array of data items.
			
			int size = cq_size(*q);
			data ** temp;
			for(;size>0;size--){
				temp = cq_peek(*q);
				//compare the two
				for(x=0;x<len; x++){
					if ( (data *)(dat[x]) != NULL){
						switch (c =form1[x]) {
							case 'i':
								result += dat[x]->i - temp[x]->i;
								break;
							case 's':
								result+= strcmp((dat[x])->s ,(temp[x])->s);					
								break;
							default:
								fprintf(stderr,"error");
								pthread_mutex_unlock(&write_mutex);
								return 0;
								break;
						}
					}
					if(result)
						break;
				}						
				if( !result ){				
					//get rid of dat
					for (x=0;x<len;x++){
						if(form1[x] =='s' && dat[x]){
							free(dat[x]->s);
						}
						free(dat[x]);
					}
					free(dat);	
					dat = cq_deq(*q);
					result = 1;
					break;
				}
				cq_rot(*q);	
				result = 0;			
			}
		}
		if (!result)
			pthread_cond_wait(&change,	&write_mutex);
		else 
			break;
		
		
	} while (1);
	
	//if we found a match, we need to copy the pointers coressponding to the ?'s 
	len = strlen(formc);
	
	
	for(x = 0;x<len;x++){
		
		if (formc[x] == '?'){
			formc++;
			
			switch (c = formc[x]) {
				case 'i':
					*(int *)(pointers[x]) = dat[x]->i;
					
					break;						
				case 's':	
					strcpy(pointers[x],dat[x]->s);
					
					break;
				default:
					fprintf(stderr,"inp: invalid format");
					pthread_mutex_unlock(&write_mutex);
					return 0;
					break;
			}
		}
		
		
	}
	
	//get rid of dat
	len = strlen(form1);
	
	for (x=0;x<len;x++){
		if(form1[x] =='s' && (dat[x]!=NULL)){
			free(dat[x]->s);
			free(dat[x]);
			
		}
		if(form1[x] =='i' && (dat[x]!=NULL)){
			free(dat[x]);
			
		}
	}
	
	
	pthread_mutex_unlock(&write_mutex);
	
	
	
	/**************end critical section***************/
	
	
	
	
	free(dat);	
	free(form1);
	va_end(ap);
	
	return result;
	
	
	
}

int rd (char * form,...){
	pthread_once(&once_control, init);
	char c;
	
	int tempi,x;
	//char* tempc;
	char* segment;
	
	char* formc = form;
	
	int len = strlen(form);
	char* form1 = malloc( sizeof (char) * len +1);
	
	
	data ** dat= malloc(sizeof(data *) *len);
	//data * tempd;
	
	//an array to hold the pointers that we may need to use;
	void * pointers[len];
	
	
	va_list ap;
	va_start(ap, form);
	
	//iterate through: construct the "no ?" string, construct the data array. 
	for(x=0;x<len;x++){
		
		
		switch (c = *form) {
			case 'i':
				tempi = va_arg(ap, int);
				
				//tempd = malloc(sizeof (data));
				//tempd->i = tempi;
				dat[x] = malloc(sizeof (data));
				dat[x]->i = tempi;
				form1[x] = c;
				
				break;
				
			case 's':
				segment = va_arg(ap, char*);
				//tempc = malloc(sizeof(char)* strlen(segment) +1);
				//tempc = strcpy(tempc,segment);
				
				//tempd = malloc(sizeof (data));
				//tempd->s = tempc;
				
				
				dat[x] = malloc(sizeof (data));
				dat[x]->s = malloc(sizeof(char)* strlen(segment) +1);
				dat[x]->s = strcpy(dat[x]->s,segment);
				
				form1[x] = c;
				
				break;
				
				
			case '?':
				pointers[x] = va_arg(ap, void*);
				dat[x] = NULL;
				len--;
				
				form++;
				
				
				form1[x] = *form;
				
				
				break;
				
				
			default:
				fprintf(stderr,"inp:invalid format string");
				return -1;
				
				break;
		}
		
		form++;
	}
	
	form1 = realloc(form1,sizeof(char)*len+1);
	form1[x] = '\0';
	int result = 0;
	
	pthread_mutex_lock(&write_mutex);
	/***************Critical section *****************/
	do{
		
		
		cirq * q = ht_get(h, form1);
		int result = 0;
		
		if(q){
		
		//search the queue for a matching array of data items.
		
		int size = cq_size(*q);
		data ** temp;
		for(;size>0;size--){
			temp = cq_peek(*q);
			//compare the two
			for(x=0;x<len; x++){
				if ( (data *)(dat[x]) != NULL){
					switch (c =form1[x]) {
						case 'i':
							result += dat[x]->i - temp[x]->i;
							break;
						case 's':
							result+= strcmp((dat[x])->s ,(temp[x])->s);					
							break;
						default:
							fprintf(stderr,"error");
							pthread_mutex_unlock(&write_mutex);
							return 0;
							break;
					}
				}
				if(result)
					break;
			}						
			if( !result ){				
				//get rid of dat
				for (x=0;x<len;x++){
					if(form1[x] =='s' && dat[x]){
						free(dat[x]->s);
					}
					free(dat[x]);
				}
				free(dat);	
				dat = cq_peek(*q);
				result = 1;
				break;
			}
			cq_rot(*q);	
			result = 0;			
		}
		}
		if (!result)
			pthread_cond_wait(&change,	&write_mutex);
		else 
			break;
		

	} while (1);
	
	//if we found a match, we need to copy the pointers coressponding to the ?'s 
		len = strlen(formc);
		
		
		for(x = 0;x<len;x++){
			
			if (formc[x] == '?'){
				formc++;
				
				switch (c = formc[x]) {
					case 'i':
						*(int *)(pointers[x]) = dat[x]->i;
						
						break;						
					case 's':	
						strcpy(pointers[x],dat[x]->s);
						
						break;
					default:
						fprintf(stderr,"inp: invalid format");
						pthread_mutex_unlock(&write_mutex);
						return 0;
						break;
				}
			}
			
			
		}
	
	//get rid of dat
	len = strlen(form1);

	for (x=0;x<len;x++){
		if(form1[x] =='s' && (dat[x]!=NULL)){
			free(dat[x]->s);
			free(dat[x]);
			
		}
		if(form1[x] =='i' && (dat[x]!=NULL)){
			free(dat[x]);
			
		}
	}
	
	
	pthread_mutex_unlock(&write_mutex);
	
	
	
	/**************end critical section***************/
	
	
	
	
	free(dat);	
	free(form1);
	va_end(ap);
	
	return result;
	
	
	
}

void init (){	
	pthread_mutex_init(&write_mutex, NULL);
	pthread_mutex_init(&read_mutex, NULL);
	h = ht_alloc(MAXSIZE);
	//pthread_cond_init(&change,NULL);
	
	return ;
}

void test1(){
	int x = 0;
	in("s?i","length",&x);
	fprintf(stderr,"x is %d",x);
}

void test2(){
	out("si","length",6);
}

char * repeat(char * string){
	char * ret = malloc(sizeof(char)*strlen(string)+1);
	strcpy(ret,string);
	return ret;
}
