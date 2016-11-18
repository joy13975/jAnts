#ifndef _SOLUTION_HH_
#define _SOLUTION_HH_

#include "util.h"
#include "route.h"

class Solution
{
public:
    Solution();
    virtual ~Solution() {};

    //class should not contain a route field
    //because if the solution changes, route becomes invalid.
    Route generateRoute();

private:
};

#endif /* include guard */