#include <cstdlib>

#include "jrng.h"

namespace jRNG
{
int rand(unsigned int& seed)
{
    return rand_r(&seed);
}

float frand(unsigned int& seed)
{
    return static_cast <float> (rand(seed)) / static_cast <float> (RAND_MAX);
}
}
