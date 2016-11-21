#ifndef _ANTS_H_
#define _ANTS_H_
#include <sstream>

#include "route.h"
#include "spec.h"
#include "savings.h"

#define DEFAULT_ALPHA       1.0f    //importance of pheromone trace
#define DEFAULT_BETA        1.0f    //importance of bias
#define DEFAULT_PHEROMONE   1.0f
#define DEFAULT_PERSISTENCE 0.975f

class Ants
{
public:
    Ants(const Spec& spec,
         const long popSize,
         std::stringstream& dataStream);
    virtual ~Ants() {};
    void search(Route& bestRoute);

private:
    const Spec& mySpec;
    const Nodes& myNodes;
    const int N;
    const int vCap;

    const long populationSize;
    const int neighbourhood;
    const float alpha, beta, persistence;

    Cache<float> realDists, pheros;
    Savings::Savings savings;

    Ints walk(unsigned int& seed);
};

#endif /* include guard */