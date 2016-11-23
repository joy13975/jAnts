#include <limits>
#include <iomanip>

#include "basic_random.h"
#include "util.h"
#include "jrng.h"
#include "output_writer.h"
#include "omp.h"
#include "score.h"
#include "util.h"

namespace BasicRandom
{

void search(Route& bestRoute,
            const Spec& spec,
            const long populationSize,
            std::stringstream& dataStream)
{
    float bestScore = std::numeric_limits<float>::max();

    Cache<float> realDists = Score::makeScoreCache(spec.getNodes(), Score::real);

    #pragma omp parallel
    {
        unsigned int tseed = spec.rand_seed + omp_get_thread_num();

        #pragma omp for
        for (int i = 0; i < populationSize; i++)
        {
            //generate a random route
            Route r = Route(spec.getNodes(), spec.getVCap(), tseed);

            //evaluate route
            float myScore = r.calcScoreWithCache(realDists);

            #pragma omp critical
            {
                //better than best?
                if (myScore < bestScore)
                {
                    bestScore = myScore;
                    bestRoute = r;
                }

                dataStream << i << " " << std::fixed << std::setprecision(4) << bestScore << "\n";
            }
        }
    }
}

}
