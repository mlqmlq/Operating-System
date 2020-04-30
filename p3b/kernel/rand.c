#include "rand.h"

unsigned int a = 1;

void xv6_srand (unsigned int seed) {
    a = seed;
}

int xv6_rand (void) {
    unsigned int x = a;
    x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
    a = x;
    return x % XV6_RAND_MAX;
}
