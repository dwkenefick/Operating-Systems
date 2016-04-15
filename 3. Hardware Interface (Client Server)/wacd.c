/*
 *  wacd.c
 * server side implementation 
 *
 *  Created by Daniel Kenefick on 5/6/11.
 *  Copyright 2011 Williams College. All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>

#define SERVER_INIT 1;
#define SERVER_SET 2;
#define SERVER_GET 3;
#define SERVER_MOMENT 4;
#define SERVER_GO 5;
#define SERVER_STOP 6;
#define SERVER_GOTO 7;
#define SERVER_FINISH 8;
#define SERVER_SHUTDOWN 9;

#define STD_PORT 100010	//the standard port number
#define TICK_TIME 75000 //nanoseconds it takes to tick the clock
#define PARALEL_PORT_OFFSET 0x378	//offset of the printer's paralel port. 

/*the length of the moment of time, in miliseconds*/
int moment = 1000000;

//internal representation of time
int time = 0;


//flasg for connection with client
char connected=0;

//flag for a clock that is on
char started=0;

//determines wheter or not we are "waiting" after a goto comand.
char waiting=0;

//the time we are waiting until after a goto command
int until=0;

//a number to keep track of whether we are "ticking" or "tocking" this moment.
char tick = 0;

void advance(void);

int main(int argc, char *argv[])
{
	int net, client;
	int buffer[2];
	struct sockaddr_in saddr, caddr;
	
	int command;
	
	/* port defaults to 10010 */
	int port =  (argc > 1) ? atoi(argv[1]):STD_PORT;

	buffer[0] = 0; buffer[1] = 0;
	
	/* open a new service socket */
	net = socket(AF_INET, SOCK_STREAM, 0);
	if (net < 0) { perror("opening socket"); exit(1); }
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = INADDR_ANY;
	saddr.sin_port = htons(port); /* host to network short */
	
	
	//make this net connection unblocking.
	fcntl(net, F_SETFL, (fcntl(net, F_GETFD) | O_NONBLOCK));
	
	
	/* bind the socket to an address */
	if (bind(net, (struct sockaddr*)&saddr, sizeof(saddr)) < 0) {
		perror("bind");
	}
	
	do{
	/* now, listen to the socket for requests  */
	if(!connected){
	listen(net,1);

	/* connection arrives
	 * caddr and cl are filled by accept to hold client address info
	 * client is the session socket
	 */
	int cl = sizeof(caddr);
	client = accept(net, (struct sockaddr*)&caddr, &cl);
	/* a non-blocking socket might /EWOULDBLOCK (see errno.h) */
		if (client < 0 && errno != EWOULDBLOCK ){
			perror("accept"); 
			exit(1); 
		}
	

	//make this client connection unblocking.
	fcntl(client, F_SETFL, (fcntl(client, F_GETFD) | O_NONBLOCK));
	}
	
	//serve incoming requests
	int temp, result, n;
	

		
		//interperate incoming commands
		if(!waiting){
			
			buffer[0] = 0; buffer[1]=0;
			n = read(client,buffer,sizeof(int)*2);
			
			command = buffer[0];
			
			//I couldnt get it to switch on the values of the #define constants above.
			switch (command) {
			case (2):
			temp = buffer[1];  
				result = -1;				
				
					if (temp<=60*60*24&& temp>=0){result = 0; time = temp; connected=1;}
				
				buffer[0] = result;
			
				buffer[1] = 0;
				
				n = write(client,buffer,sizeof(int) * 2);
				
				if (n < 0) {
					perror("set:write");
 
				}
				
				break;
								
				case 3 :
				buffer[0] = time;
				buffer[1] = 0;
				
				n = write(client,buffer,sizeof(int) * 2);
				
				if (n < 0) {
					perror("get:write");
 
				}
				
				break;

			case 4:
				temp = buffer[1]; 
					temp *= 1000;
				result = -1;				
				
				if (temp>=50){result = 0; moment = temp;}
				
				buffer[0] = result;
				buffer[1] = 0;
				
				n = write(client,buffer,sizeof(int) * 2);
				
				if (n < 0) {
					perror("moment:write");

				}
				break;
				
			case 5:
				started = 1;
				buffer[0] = 0;
				buffer[1] = 0;
				
				n = write(client,buffer,sizeof(int) * 2);
				
				if (n < 0) {
					perror("go:write");
 
				}
				
				break;

			case 6:
				started = 0;
				buffer[0] = 0;
				buffer[1] = 0;
				
				n = write(client,buffer,sizeof(int) * 2);
				
				if (n < 0) {
					perror("stop:write");
 
				}
				
				break;
				
			case 7:
				temp = buffer[1];  
				result = -1;				
				
				if (((time+3600)%(3600*24))>=temp){result = 0; waiting = 1; until = temp;}
				
					
				buffer[0] = result;
				buffer[1] = 0;
					
					
				break;

			case 8:
				buffer[0] = 0;
				buffer[1] = 0;
					
				n = write(client,buffer,sizeof(int) * 2);
					
				if (n < 0) {
					perror("finish:write");
						
				}		
				close(client);
				
				break;
					
				
			
			case 9: // shutddown
					
					buffer[0] = 0;
					buffer[1] = 0;
					
					n = write(client,buffer,sizeof(int) * 2);
					
					if (n < 0) {
						perror("shutdown:write");
						
					}		
					close(client);
					close(net);
					return 0;
					
					break;
					
					
			case 0: //no command
					break;
					
			default:
				printf("Error, unrecognised command: %d",command);
				break;
		}
		}
		
		//rest a moment
		usleep(moment-TICK_TIME);
		
		//incrememnet the clock
		if(started){
			advance();
			if (until&& time==until){
				waiting = 0; 
				until = 0;
				n = write(client,buffer,sizeof(int) * 2);
				
				if (n < 0) {
					perror("goto:write");
					
				}	
				
			}
		}
		
		/*print*/
		char * AMPM = "PM";
		int seconds = time%60;
		int minutes = (time/ 60)%60;
		int hours = (time - (minutes*60) - seconds)/(60*60);
		if(hours>12){hours = hours-12;AMPM="PM";} 
		printf("time is: %d:%d:%d%s\n",hours,minutes,seconds,AMPM);
	}while(1);
	
	return 0;	
		
}		

void advance(void){
	
	
	
	//advance the internal representation of time. 
	time++;
	time = time% (60*60*24);
	
		fprintf(stderr,"opening paralel port\n");
		int pp = open("/dev/port",O_WRONLY);
		if(pp<0){
			perror("advance:open");exit(1);
		}
		
		fprintf(stderr,"seeking to paralel port offset\n");
		lseek(pp,PARALEL_PORT_OFFSET,SEEK_SET);
		
	char next;
		if(tick == 0){
			next = 1;
			
			fprintf(stderr,"writing %c\n",next);
			
			write(pp,&next,1);
			tick = 1;
		}else{
			next = 2;
			
			fprintf(stderr,"writing %c\n",next);
			
			write(pp,&next,1);  
			tick = 0;
		}
		
		usleep(TICK_TIME);
	next = 3;
		fprintf(stderr,"writing %c\n",next);
		write(pport,&next,1);
		
		close(pp);
	
		return 0;
	
	
}



		  
