#include <stdio.h>
#include <stdlib.h>

// radius of circle
#define RADIUS 10000

/* reference:
 * http://en.wikipedia.org/wiki/Linear_feedback_shift_register
 * 32-bit maximal period Galois LFSR.
*/
unsigned int myrandom() {
    static unsigned int lfsr = 1234569;

    lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xD0000001u);
    return lfsr;
}

int main(int argc, char *argv[])
{
   int i, its, hits = 0;
   unsigned int i1, i2;

   if (argc != 2) {
      fprintf(stderr, "Usage: %s <iterations>\n", argv[0]);
      exit(0);
   }

   its = atoi(argv[1]);
   //srandom(1);
   for (i = 0; i < its; i++) {
       i1 = myrandom() % RADIUS;
       i2 = myrandom() % RADIUS;
       if ((i1*i1 + i2*i2) <= RADIUS*RADIUS)
           hits++;
   }
   printf("%d.%02d\n", ((hits*4*100) / its)/100, (((hits*4*100) / its))%100);
   return 0;
}

