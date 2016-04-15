/*
 * Utility routines that are useful to supporting Williams Unix File System
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wufs_fs.h"
#include "wufs.h"

/*
 * Compute (rounding up) chunks of size chunkSize needed to hold size items.
 * Might be used to find out how many blocks we need for i inodes:
 *    fullChunks(i,WUFS_BLOCKSIZE/WUFS_INODESIZE);
 */
int fullChunks(int size, int chunkSize)
{
  return (size + (chunkSize-1))/chunkSize;
}

/*
 * Some bitfield manipulation functions
 * Accidentally deleted (*sigh*) by Duane.
 */

/*
 * Set the ith bit (zero origin) in bitfield f
 */
void setBit(__u8 *f, int i)
{
	*f = *f | (1 << i);
}

/*
 * Clear the ith bit (zero origin) in bitfield f
 */
void clearBit(__u8 *f, int i)
{
	*f = *f & ( 0xff - (1 << i)); 
}

/*
 * Return the ith bit (zero origin) in bitfield field: 0 or 1
 */
int getBit(__u8 *field, int i)
{
  return ( *field & ( 1 << i )> 0 ;
}

/*
 * Find the next bit (starting with i or, if -1, 0) set in field of n.
 * If none, return -1.
 */
int findNextSet(__u8 *f, int i, int n)
{
	f += i/8;	//seek the address
	int m =i%8;		//which number bit are we starting at in the first block?
	
	
	
	for(;i<n;f++){
		if( *f & 0xff){
			//there's a bit set in this byte, find it
			for(;(m<8)&&(i<n);i++,m++){
				if(*f&(1<<m))
					return i;		
			}
		} else {
			no bit set, increment.
			i+=8;
		}
		m = 0;
	}
	return -1;
}

/*
 * Find the next bit (starting with i or, if -1, 0) clear in field of n.
 * If none, return -1.
 */
int findNextClear(__u8 *f, int i, int n)
{
	f += i/8;	//seek the address
	int m =i%8;		//which number bit are we starting at in the first block?
	
	
	
	for(;i<n;f++){
		if( *f < 0xff){
			//there's a bit set in this byte, find it
			for(;(m<8)&&(i<n);i++,m++){
				if( !(*f&(1<<m)) )
					return i;		
			}
		} else {
			no bit set, increment.
			i+=8;
		}
		m = 0;
	}
	return -1;
	
}
