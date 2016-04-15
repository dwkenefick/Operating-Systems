#include <string.h>
#include "hash.c"
#include <stdarg.h>
#include <stdio.h>

#define MAXSIZE 1000
#define MAX_TUPLE_LENGTH 10;

int out(char* form,...);
int inp(char *form,...);
int rdp (char * form, ...);


hash h;

typedef union{
	char * s;
	int i;

} data;

void add (data ** dat, char * key);

int out(char * form,...){
	va_list ap;
	va_start(ap,form);
	
	
	int len = strlen(form);
	if(len == 0){
		fprintf(stderr,"out: invalid format, must have non-zero lenght");
		return -1;
	}
	
	/*
	if( len > MAX_TUPLE_LENGTH ){
		fprintf(stderr,"out:format too long. at most 10 items");
		return -1;
	}
	*/
	char * formc = form;
	int x, count;
	char c;
	
	int tempi;
	char* tempc;
	char* segment;
	data ** dat = malloc(sizeof(data*) *len) ;
	data * tempd;
	
	count = 0;
	
	for(x=0;x<len;x++){
		
		
		switch (c = *form) {
			case 'i':
				tempi = va_arg(ap, int);
				
				tempd = malloc(sizeof (data));
				tempd->i = tempi;
				dat[count] = tempd;
				count++;
				
				break;
				
			case 's':
				segment = va_arg(ap, char *);
				tempc = malloc(sizeof(char)* strlen(segment) +1);
				tempc = strcpy(tempc,segment);
				
				tempd = malloc(sizeof (data) );
				tempd->s = tempc;
				
				
				dat[count] = tempd;
				count++;
				
				break;
				
			default:
				fprintf(stderr,"out in ts:invalid format string");
				return -1;
				
				break;
		}
		
		
		form++;
	}
	
	va_end(ap);
	add(dat,formc);

	return 0;		
}

//RW

/*
 Process: 
 get cq for the provided format
 construct data array
 search cq for the data array, return the match
 copy any variable args
 
 */

int inp (char *form,...){
	
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
	
	if(!q)
		return 0;
	
	
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
						return 0;
						break;
				}
			}
			
			
		}
	}
	
	//get rid of dat
	for (x=0;x<len;x++){
		if(form1[x] =='s' && dat[x]){
			free(dat[x]->s);
		}
		//free(dat[x]);
	}
	
	free(dat);	
	free(form1);
	va_end(ap);
	
	return result;
			
}












//Read only
int rdp (char *form, ...){
	
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
	
	if(!q)
		return 0;
	
	
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
	for (x=0;x<len;x++){
		if(form1[x] =='s'){
			free(dat[x]->s);
		}
		//free(dat[x]);
	}
	
	free(dat);	
	free(form1);
	va_end(ap);
	
	return result;
	
}


//in case I want to change the data structure later

//add the tuple to the CQ of matching 
void add(data ** dat, char* key){
	if (!h){
		h = ht_alloc(MAXSIZE);
	}
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





