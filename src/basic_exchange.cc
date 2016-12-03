#include <limits>
#include <iomanip>
#include <algorithm>

#include "basic_exchange.h"
#include "util.h"
#include "jrng.h"
#include "output_writer.h"
#include "omp.h"
#include "score.h"
#include "util.h"

namespace BasicExchange
{

void search(Route& bestRoute,
            const Spec& spec,
            std::stringstream& dataStream,
            double startTime)
{
    const Nodes& nodes = spec.getNodes();
    const int dim = spec.getDim();
    const int vcap = spec.getVCap();
    int itr = 0;

    Cache<float> myDists = Score::makeScoreCache(spec.getNodes(), Score::real);

    Ints sol = Route::genAscendHops(dim);
    float bestScore = std::numeric_limits<float>::max();
    int maxStag = 20, stag = 0;

    while (stag < 20)
    {

        for (int i = 0; i < dim - 1; i++)
        {
            for (int j = i + 1; j < dim - 1; j++)
            {
                if (i == j)
                    continue;

                const int node1 = sol[i], node2 = sol[j];

                // Could putting node i in position j make a better route? (i < j)
                Ints newSol1 = Ints(sol.begin(), sol.end());
                newSol1.insert(newSol1.begin() + j, node1);
                newSol1.erase(newSol1.begin() + i);
                const float c1 = Route(nodes, sol, vcap).calcScoreWithCache(myDists);
                Route newRoute1 = Route(nodes, newSol1, vcap);
                const float c2 = newRoute1.calcScoreWithCache(myDists);

                // Could putting node j in position i make a better route? (i < j)
                Ints newSol2 = Ints(sol.begin(), sol.end());
                newSol2.erase(newSol2.begin() + j);
                newSol2.insert(newSol2.begin() + i, node2);
                Route newRoute2 = Route(nodes, newSol2, vcap);
                const float c3 = newRoute2.calcScoreWithCache(myDists);

                bestScore = std::min(std::min(c1, c2), c3);
                if (bestScore == c2)
                {
                    sol = newSol1;
                    bestRoute = newRoute1;
                }
                else if (bestScore == c3)
                {
                    sol = newSol2;
                    bestRoute = newRoute2;
                }
            }
        }

        for (int i = 0; i < dim - 1; i++)
        {
            for (int j = i + 1; j < dim - 1; j++)
            {
                if (i == j)
                    continue;

                // in order up to i, reverse up to j, then in order
                Ints newSol = sol;
                std::reverse(newSol.begin() + i, newSol.begin() + j + 1);

                const Route newRoute = Route(nodes, newSol, vcap);
                const float newScore = newRoute.calcScoreWithCache(myDists);
                if ( newScore < bestScore )
                {
                    stag = 0;
                    sol = newSol;
                    bestRoute = newRoute;
                    bestScore = newScore;
                }

                dataStream << i << " " << std::fixed << std::setprecision(4) << bestScore << "\n";
            }
        }

        msg("itr# %5.d, best %6.4f, time %6.2f\n",
            itr,
            bestScore,
            (get_timestamp_us() - startTime) / 1e6);

        itr++;
        stag++;
    }
}

}
