#include <cstdlib>

#include "jrand.h"

int jrand(int lim)
{
    return rand() % lim;
}