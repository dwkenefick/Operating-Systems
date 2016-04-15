#include <stdio.h>
#include <ctype.h>
#include "ts.h"
int main()
{
  char word[512], *fp, *tp;
  int count;
  // scan words, sans punctuation
  while (1 == scanf("%s",word)) {
    for (fp = tp = word; (*tp = *fp); fp++) isalpha(*tp) && (*tp++ |= 32);
    count = 0;
    inp("s?i",word,&count);	// read in tuple (if none, count untouched)
    count++;
    out("si",word,count);	// write out tuple
  }
  count = 0;
  while (inp("?si",word,1)) count++;
  printf("%d words appear once\n",count);
  return 0;
}
