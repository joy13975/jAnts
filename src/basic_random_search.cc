#include <limits>
#include <sstream>
#include <iomanip>

#include "basic_random_search.h"
#include "util.h"
#include "jrand.h"
#include "output_writer.h"

Route basicRandomSearch(const Spec& spec, const long n)
{
    Route bestRoute = Route(spec);//dummy
    float bestScore = std::numeric_limits<float>::max();
    const int dim = spec.getDim();

    std::stringstream ss;

    #pragma omp parallel for
    for (int i = 0; i < n; i++)
    {
        //generate a random route
        Route r = Route(spec, jrand);

        //evaluate route
        float myScore = r.calcScore();

        #pragma omp critical
        {
            //better than best?
            if (myScore < bestScore)
            {
                bestScore = myScore;
                bestRoute = r;
            }

            ss << i << " " << std::fixed << std::setprecision(4) << bestScore << "\n";
        }
    }

    write_general("data.txt", ss);

    return bestRoute;
}