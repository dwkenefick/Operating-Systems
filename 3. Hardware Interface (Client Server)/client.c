/*
 *  client.c
 *  
 *
 *  Created by Daniel Kenefick on 5/5/11.
 *  Copyright 2011 Williams College. All rights reserved.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 



/*
 Codes for the different methods. These are sent to the server via write() to indicate which action the client can take. 
 There are  9
 */

#define SERVER_INIT 1;
#define SERVER_SET 2;
#define SERVER_GET 3;
#define SERVER_MOMENT 4;
#define SERVER_GO 5;
#define SERVER_STOP 6;
#define SERVER_GOTO 7;
#define SERVER_FINISH 8;
#define SERVER_SHUTDOWN 9;

// flag for connected
int connected =0;

//the net number, which we will need to write to the server. 
int net = 0;

/*
 Establish a conection with the WACD server. Sucsess returns 0, failure -1
 */
int wac_init(char *hostname);

/*
 This sets the time on the clock to secs seconds past 12 o’clock.
 returns immediately with -1 or 0 in failure . sucsess.
 */
int wac_set(int secs);

/*
 This routine immediately returns the current time of the clock as the number of seconds past 12.
 Failure is indicated by  -1.
 */
int wac_get(void);

/*
 The status returned should reflect the success of changing this internal value.
 */
int wac_moment(int msecs); 

/*
 This procedure starts the clock. The return value indicates success or failure.
 */
int wac_go();

/*
 Stops the clock.
 */
int wac_stop();

/*
 This command starts the clock (if necessary) and returns when the clock reads time seconds past 12 o’clock.
 This command will fail if time is more than 3600 moments (an “hour”) ahead of the current time. 
 While the clock is going to the target time, the daemon does not process other commands.
 When the clock reaches the desired time, the clock is stopped and the procedure returns 0, indicating success.
 */
int wac_goto(int time);

/*
 This closes the connection to the server. The server does not exit. It continues to maintain the time
 according to its internal state.
 */
int wac_finish();

/*
 This command is like wac_finish but causes the daemon to gracefully exit. The return value is 0 on success, -1 otherwise.
 */
int wac_shutdown();  





int wac_init(char *hostname){
	
	if(connected) return 0;
	
	int n;
    struct sockaddr_in saddr;  /* an ip(4) socket address */
    struct hostent *server;
    char buffer[256];
    /* host defaults to localhost, port to 10010 */
    char *hostname = (argc>1)?argv[1]:"localhost";
    int port = (argc>2)?atoi(argv[2]):10010;
	
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
	
	
	if(!connected){
		printf("wac_set: not connected to server");
		return -1;
	}
	
	//seconds should not exceed the number of seconds in a day.
	secs = secs%(60*60*24)
	
	
	buffer = {SERVER_SET,secs};
	
	/* write to the buffer*/
	n = write(net,buffer,sizeof(int) * 2);
	
	if (n < 0) {
		perror("set:write");
		return -1;
    }
	
	/*read the awnser back - read two for symetry, and to make sure the message queue is clean.*/
	n = read(net,buffer,sizeof(int));
    if (n < 0) {
		perror("set:read");
		return -1;
    }
	
	/*retun the server's sucsess or failure*/
	return buffer[1];
	
}

int wac_get(void){

	int buffer [2];
	
	
	if(!connected){
		printf("wac_get: not connected to server");
		return -1;
	}
	

	buffer = {SERVER_GET,0};
	
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
	return buffer[1];
	
}

int wac_moment(int msecs){
	int buffer [2];
	
	
	if(!connected){
		printf("wac_moment: not connected to server");
		return -1;
	}
	
	//seconds should not exceed the number of seconds in a day.
	secs = secs%(60*60*24)
	
	
	buffer = {SERVER_SET,secs};
	
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
	return buffer[1];
	
	
}

int wac_go(){
	
	int buffer [2];
	
	
	if(!connected){
		printf("wac_go: not connected to server");
		return -1;
	}
	
	
	buffer = {SERVER_GO,0};
	
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
	
	/*retun the server's sucsess or failure*/
	return buffer[1];
}

int wac_stop{
	
	int buffer [2];
	
	
	if(!connected){
		printf("wac_stop: not connected to server");
		return -1;
	}
	
	
	buffer = {SERVER_STOP,0};
	
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
	
	/*retun the server's sucsess or failure*/
	return buffer[1];	
}

int wac_goto(int time){
	int buffer [2];
	
	
	if(!connected){
		printf("wac_set: not connected to server");
		return -1;
	}
	
	//seconds should not exceed the number of seconds in a day.
	secs = secs%(60*60*24)
	
	
	buffer = {SERVER_GOTO,secs};
	
	/* write to the buffer*/
	n = write(net,buffer,sizeof(int) * 2);
	
	if (n < 0) {
		perror("set:write");
		return -1;
    }
	
	/*read the awnser back - read two for symetry, and to make sure the message queue is clean.*/
	
	/*this should block until something is written, or the sucsess / failure */
	n = read(net,buffer,sizeof(int));
    if (n < 0) {
		perror("set:read");
		return -1;
    }
	
	/*retun the server's sucsess or failure*/
	return buffer[1];
	
	
}

