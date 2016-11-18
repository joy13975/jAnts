#include <limits>

#include "basic_random_search.h"
#include "util.h"
#include "jrand.h"

Route basicRandomSearch(const Spec& spec, const long n)
{
    Route bestRoute = Route(spec);//dummy
    float bestScore = std::numeric_limits<float>::max();
    const int dim = spec.getDim();

    for (int i = 0; i < n; i++)
    {
        //generate a random route
        Route r = Route(spec, jrand);

        //evaluate route
        float myScore = r.calcScore();

        //better than best?
        if (myScore < bestScore)
        {
            bestScore = myScore;
            bestRoute = r;
        }

        //output best score
        msg("Route #%d: %.2f (best=%.2f) (end=%d)\n", i + 1, myScore, bestScore, r.getHops()[dim - 2]);
    }

    return bestRoute;
}