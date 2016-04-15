/*
 * Implementation of a "standard" clock.
 */
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include "wac.h"

void t(int *now)
{
  *now = time(0) - 4*60*60;
  *now = *now % (12*60*60);
}

int main(int argc, char**argv)
{
  int now,i;
  wac_init(argc>1?argv[1]:"localhost");
  t(&now);
  printf("the error code is: %d\n",wac_set(now));
  wac_moment(500);
  wac_go();
	sleep(2);
	printf("seconds:%d\n",wac_get());
	sleep(2);
	
	wac_goto(wac_get()+10);
	
  wac_shutdown();
}
