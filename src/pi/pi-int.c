#include <stdio.h>
#include <stdlib.h>

#ifdef CUSTOM_RANDOM
#include "random.c"
#endif

#define PRECISION 3
// radius of circle, must be 10^PRECISION
#define RADIUS 1000

int main(int argc, char *argv[])
{
   unsigned int i, its, hits = 0;
   unsigned int i1, i2;

   if (argc != 2) {
      fprintf(stderr, "Usage: %s <iterations>\n", argv[0]);
      exit(0);
   }

   its = atoi(argv[1]);
   srandom(1);
   for (i = 0; i < its; i++) {
       i1 = random() % RADIUS;
       i2 = random() % RADIUS;
       if ((i1*i1 + i2*i2) <= RADIUS*RADIUS)
           hits++;
   }
   
   unsigned int approx = (hits*4*RADIUS / its);
   unsigned int whole = approx / RADIUS;
   unsigned int decimal = approx % RADIUS;

#define make_format_(p) "%u.%0" #p "u\n"
#define make_format(p) make_format_(p)
   printf(make_format(PRECISION), whole, decimal);
   return 0;
}

