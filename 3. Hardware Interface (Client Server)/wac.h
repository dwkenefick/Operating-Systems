/*
 *  wac.h
 *  header file for the WAC client / server interface.
 *
 *  Created by Daniel Kenefick on 5/6/11.
 *  Copyright 2011 Williams College. All rights reserved.
 *
 */

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
int wac_go(void);

/*
 Stops the clock.
 */
int wac_stop(void);

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
int wac_finish(void);

/*
 This command is like wac_finish but causes the daemon to gracefully exit. The return value is 0 on success, -1 otherwise.
 */
int wac_shutdown(void);  
