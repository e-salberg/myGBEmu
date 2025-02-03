#include <stdio.h>
#include <stdlib.h>

#define CHECK_BIT(a, n) ((a & (1 << n)) ? 1 : 0)
#define SET_BIT(a, n, on) { if (on) a |= (1 << n); else a &= ~(1 << n);}

#define NO_IMPL { fprintf(stderr, "NOT YET IMPLEMENTED\n"); exit(-5); }