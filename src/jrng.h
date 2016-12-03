#ifndef _JRNG_H_
#define _JRNG_H_

#include <iterator>

namespace jRNG
{
int rand(unsigned int& seed);

float frand(unsigned int& seed);

template< class RandomIt >
void random_shuffle(unsigned int& seed, RandomIt first, RandomIt last)
{
    typename std::iterator_traits<RandomIt>::difference_type i, n;
    n = last - first;
    for (i = n - 1; i > 0; --i) {
        using std::swap;
        swap(first[i], first[rand(seed) % (i + 1)]);
    }
}
}

#endif /* include guard */