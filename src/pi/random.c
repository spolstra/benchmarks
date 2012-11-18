#include <stdint.h>

unsigned long random_state = 0;

long random(void)
{
    uint32_t i;
    i = random_state;
    
/*
 * Compute x = (7^5 * x) mod (2^31 - 1)
 * wihout overflowing 31 bits:
 *      (2^31 - 1) = 127773 * (7^5) + 2836
 * From "Random number generators: good ones are hard to find",
 * Park and Miller, Communications of the ACM, vol. 31, no. 10,
 * October 1988, p. 1195.
 */
    int32_t x = i;
    int32_t hi, lo;
    
/* Can't be initialized with 0, so use another value. */
    if (x == 0)
        x = 123459876;
    hi = x / 127773;
    lo = x % 127773;
    x = 16807 * lo - 2836 * hi;
    if (x < 0)
        x += 0x7fffffff;
    random_state = i = x & 0x7fffffff;
    
    return (long)i;
}

void srandom(unsigned long x)
{
    int i;
    random_state = x;
    for (i = 0; i < 50; ++i)
        (void)random();
}
