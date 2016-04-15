/*
 *  wac.c
 *  
 *
 *  Created by Daniel Kenefick on 5/5/11.
 *  Copyright 2011 Williams College. All rights reserved.
 *
 */
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h> 
#define STD_PORT 100010

/*
 Codes for the different methods. These are sent to the server via write() to indicate which action the client can take. 
 There are  9
 */

#define SERVER_INIT 1;
#define SERVER_SET 2;
#define SERVER_GET	3;
#define SERVER_MOMENT 4;
#define SERVER_GO 5;
#define SERVER_STOP 6;
#define SERVER_GOTO 7;
#define SERVER_FINISH 8;
#define SERVER_SHUTDOWN 9;

// flag for connected
char connected =0;
//flag for started
char started=0;

//the net number, which we will need to write to the server. 
int net = 0;


int wac_init(char *hostname){
	
	if(connected) return 0;
	
    struct sockaddr_in saddr;  /* an ip(4) socket address */
    struct hostent *server;
    /* host defaults to localhost, port to 10010 */
    //char *hostname = (argc>1)?argv[1]:"localhost";
    int port = STD_PORT;
	
	/* get server address */
    server = gethostbyname(hostname);
    if (server == NULL) {
		perror("gethostbyname");
		return -1; 
    }
	
	/* Create appropriate TCP stream socket */
    net = socket(AF_INET, SOCK_STREAM, 0);
    if (net < 0) {
		perror("socket");
		return -1;
    }
	
	/* Connect to host on port */
    memset(&saddr,0,sizeof(saddr));
    saddr.sin_family = AF_INET;
    /* copy the host address to the socket */
    bcopy(server->h_addr, &(saddr.sin_addr.s_addr), server->h_length);
    saddr.sin_port = htons(port);
    if (connect(net,(struct sockaddr *)&saddr,sizeof(saddr)) < 0) {
		perror("connect");
		return -1;
    }
	

	/*at this point we are connected. return*/
	connected = 1;
	return 0;
	
	
}

int wac_set(int secs){
	/*
	 we could the package in 3 bytes:
	 3 bits for the operation code
	 21 bits for the max number of seconds in the day - 0b10101000110000000
	 
	 This involves less trasfer space, but at the cost of server responsiveness.
	 
	 either way, this whole thing is sent in a single TCP/IP packet, so there is no bandwidth benefit for reducing the size of
	 the information transfer.
	 */
	int buffer [2];
	int n;
	
	
	if(!connected){
		printf("wac_set: not connected to server");
		return -1;
	}
	
	//seconds should not exceed the number of seconds in a day.
	secs = secs%(60*60*24);
	
	
	buffer[0] = SERVER_SET;
	buffer[1] = secs;
	
	/* write to the buffer*/
	n = write(net,buffer,sizeof(int) * 2);
	
	if (n < 0) {
		perror("set:write");
		return -1;
    }
	
	/*read the awnser back - read two for symetry, and to make sure the message queue is clean.*/
	n = read(net,buffer,sizeof(int)*2);
    if (n < 0) {
		perror("set:read");
		return -1;
    }
	
	/*retun the server's sucsess or failure*/
	return buffer[0];
	
}

int wac_get(void){
	int n;
	int buffer [2];
	
	
	if(!connected){
		printf("wac_get: not connected to server");
		return -1;
	}
	

	buffer[0] = SERVER_GET;
	buffer[1] = 0;

	
	/* write to the buffer*/
	n = write(net,buffer,sizeof(int) * 2);
	
	if (n < 0) {
		perror("get:write");
		return -1;
    }
	
	n = read(net,buffer,sizeof(int)*2);
    
	if (n < 0) {
		perror("get:read");
		return -1;
    }
	
	/*retun the server's sucsess or failure*/
	return buffer[0];
	
}

int wac_moment(int msecs){
	int buffer [2];
	int n;
	
	if(!connected){
		printf("wac_moment: not connected to server");
		return -1;
	}
	
	
	buffer [0] = SERVER_MOMENT;
	buffer [1] = msecs;
	
	/* write to the buffer*/
	n = write(net,buffer,sizeof(int) * 2);
	
	if (n < 0) {
		perror("moment:write");
		return -1;
    }
	
	n = read(net,buffer,sizeof(int)*2);
    if (n < 0) {
		perror("moment:read");
		return -1;
    }
	
	/*retun the server's sucsess or failure*/
	return buffer[0];
	
	
}

int wac_go(void){
	
	int buffer [2];
	int n;
	
	if(!connected){
		printf("wac_go: not connected to server");
		return -1;
	}
	buffer[0] = SERVER_GO;
	buffer[1] = 0;
	
	
	/* write to the buffer*/
	n = write(net,buffer,sizeof(int) * 2);
	
	if (n < 0) {
		perror("go:write");
		return -1;
    }
	
	n = read(net,buffer,sizeof(int)*2);
    if (n < 0) {
		perror("go:read");
		return -1;
    }
	
	if(!buffer[1]) started = 1;
	/*retun the server's sucsess or failure*/
	return buffer[0];
}

int wac_stop(void){
	int n;
	int buffer [2];
	
	
	if(!connected){
		printf("wac_stop: not connected to server");
		return -1;
	}
	
	buffer[0] = SERVER_STOP;
	buffer[1] = 0;
	
	/* write to the buffer*/
	n = write(net,buffer,sizeof(int) * 2);
	
	if (n < 0) {
		perror("stop:write");
		return -1;
    }
	
	n = read(net,buffer,sizeof(int)*2);
    if (n < 0) {
		perror("stop:read");
		return -1;
    }
	
	//lower the flag, if apropriate. 
	if(!buffer[1]){
		started = 0;
	}
	
	/*retun the server's sucsess or failure*/
	return buffer[0];	
}

int wac_goto(int time){
	int buffer [2];
	int n;
	
	if(!connected){
		printf("wac_goto: not connected to server");
		return -1;
	}
	
	//start the clock if not already started, return error on failure
	if (!started){
		if(wac_go()){
			printf("wac_goto: could not start the clock");
			return -1;
		}
	}
	
	buffer[0] = SERVER_GOTO;
	buffer[1] = time;
	
	/* write to the buffer*/
	n = write(net,buffer,sizeof(int) * 2);
	
	if (n < 0) {
		perror("goto:write");
		return -1;
    }
	
	/*read the awnser back - read two for symetry, and to make sure the message queue is clean.*/
	
	/*this should block until something is written, or the sucsess / failure */
	n = read(net,buffer,sizeof(int)*2);
    if (n < 0) {
		perror("goto:read");
		return -1;
    }
	
	/*retun the server's sucsess or failure*/
	return buffer[0];
	
	
}

int wac_finish(void){
	int n;
	int buffer [2];
	
	
	if(!connected){
		printf("wac_finish: not connected to server");
		return -1;
	}
	
	buffer[0] = SERVER_FINISH;
	buffer[1] =0;
	
	/* write to the buffer*/
	n = write(net,buffer,sizeof(int) * 2);
	
	if (n < 0) {
		perror("finish:write");
		return -1;
    }
	
	n = read(net,buffer,sizeof(int)*2);
    if (n < 0) {
		perror("finish:read");
		return -1;
    }
	
	
	/*retun the server's sucsess or failure*/
	if(!buffer[0]){
	close(net);
	}
	return buffer[0];
	
}

int wac_shutdown(void){
	int n;
	int buffer [2];
	
	
	if(!connected){
		printf("wac_shutdown: not connected to server");
		return -1;
	}
	
	buffer[0] = SERVER_SHUTDOWN;
	buffer[1] = 0;
	
	/* write to the buffer*/
	n = write(net,buffer,sizeof(int) * 2);
	
	if (n < 0) {
		perror("shutdown:write");
		return -1;
    }
	
	n = read(net,buffer,sizeof(int)*2);
    if (n < 0) {
		perror("shutdown:read");
		return -1;
    }
	
	
	/*retun the server's sucsess or failure*/
	if(!buffer[0]){
		close(net);
	}
	return buffer[0];
	
	
}
