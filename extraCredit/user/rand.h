#ifndef _RAND_H
#define _RAND_H
#define	XV6_RAND_MAX  2147483647

/* 
    Return a random integer between 0 and XV6_RAND_MAX inclusive. 
    NOTE: If xv6_rand is called before any calls to xv6_srand have been made, the same 
    sequence shall be generated as when xv6_srand is first called with a seed value of 1.
*/
extern int xv6_rand (void);

/* 
    The xv6_srand function uses the argument as a seed for a new sequence of pseudo-random numbers to be returned by subsequent calls to rand.
    If xv6_srand is then called with the same seed value, the sequence of pseudo-random numbers shall be repeated.
    If xv6_rand is called before any calls to xv6_srand have been made, the same sequence shall be generated as when xv6_srand is first called with a seed value of 1.  
*/
void xv6_srand (unsigned int seed);
#endif // _RAND_H
