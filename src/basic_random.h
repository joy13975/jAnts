#ifndef _BASIC_RANDOM_H_
#define _BASIC_RANDOM_H_
#include <sstream>

#include "spec.h"
#include "route.h"

namespace BasicRandom
{

void search(Route& bestRoute,
            const Spec& spec,
            const long populationSize,
            std::stringstream& dataStream);

}

#endif /* include guard */