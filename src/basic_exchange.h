#ifndef _BASIC_EXCHANGE_H_
#define _BASIC_EXCHANGE_H_
#include <sstream>

#include "spec.h"
#include "route.h"

namespace BasicExchange
{

void search(Route& bestRoute,
            const Spec& spec,
            std::stringstream& dataStream,
            double startTime);

}

#endif /* include guard */