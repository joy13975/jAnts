#include <algorithm>
#include <vector>
#include <cmath>

#include "ants.h"
#include "score.h"
#include "util.h"
#include "typedefs.h"
#include "omp.h"
#include "jrng.h"
#include "jints.h"

typedef struct WayPoint
{
    int left, right;
    int load;
    struct WayPoint *otherEnd;
} WayPoint;
typedef std::vector<WayPoint> WayPoints;

Ants::Ants(const Spec& spec,
           const long popSize,
           std::stringstream& dataStream)
    : mySpec(spec), myNodes(spec.getNodes()), N(spec.getDim()), vCap(spec.getVCap()),
      populationSize(popSize), neighbourhood(spec.getDim() / 4),
      alpha(DEFAULT_ALPHA), beta(DEFAULT_BETA), persistence(DEFAULT_PERSISTENCE)
{
    this->realDists = Score::makeCache(this->myNodes, Score::real);
    this->pheros    = Score::makeCache(this->N, DEFAULT_PHEROMONE);
    this->savings   = Savings::makeSavings(this->realDists);
}

inline void improveRoute(Route& r)
{

}

Ints Ants::walk(unsigned int& seed)
{
    WayPoints waypoints = WayPoints(N);
    Savings::Savings lclSavings = Savings::Savings(this->savings);

    float cost = 0.0f;
    for (int i = 1; i < N; i++)
    {
        waypoints[i] = {0, 0, this->myNodes[i].z, 0};
        waypoints[i].otherEnd = &waypoints[i];
        cost += this->realDists[i][0] * 2.0f;
    }

    // Apply savings until no more feasible
    while (lclSavings.size() > 0)
    {
        // Accumulate probabilities (no need to sort)
        const int cmlProbsSize = std::min(neighbourhood, (int) lclSavings.size());
        Floats cmlProbs = Floats(cmlProbsSize, 0.0f);
        float probSum = 0.0f;
        for (int i = 0; i < cmlProbsSize; i++)
        {
            const Savings::Saving& s = lclSavings[i];
            cmlProbs[i] = (probSum += s.gain * this->pheros[s.n1][s.n2]);
        }

        // Roll dice and find corresponding id
        const float dice = jRNG::frand(seed) * probSum;
        int chosenSId = -1;
        for (int i = 0; i < cmlProbsSize; i++)
        {
            if (dice < cmlProbs[i])
            {
                chosenSId = i;
                break;
            }
        }

        if (chosenSId == -1)
            die("Could not find saving ID! cmlProbsSize=%d, dice=%.2f, probSum=%.2f\n",
                cmlProbsSize, dice, probSum);

        const Savings::Saving& chosenSaving = lclSavings[chosenSId];
        // dbg("Saving %d~%d, id: %d, savings size: %d\n",
        //     chosenSaving.n1, chosenSaving.n2, chosenSId, lclSavings.size());

        WayPoint &w1 = waypoints[chosenSaving.n1], &w2 = waypoints[chosenSaving.n2];

        // debug only
        // if (w1.left * w1.right != 0 || w2.left * w2.right != 0)
        //     die("Illegal saving: %d(%d, %d), %d(%d, %d)\n",
        //         chosenSaving.n1, w1.left, w1.right, chosenSaving.n2, w2.left, w2.right);

        // if (w1.left == chosenSaving.n2 || w1.right == chosenSaving.n2)
        //     die("Illegal saving %d~%d, n1: %d(%d, %d)\n",
        //         chosenSaving.n1, chosenSaving.n2, chosenSaving.n1, w1.left, w1.right);

        // if (w2.left == chosenSaving.n1 || w2.right == chosenSaving.n1)
        //     die("Illegal saving %d~%d, n2: %d(%d, %d)\n",
        //         chosenSaving.n1, chosenSaving.n2, chosenSaving.n2, w2.left, w2.right);

        w1.left == 0 ? w1.left = chosenSaving.n2 : w1.right = chosenSaving.n2;
        w2.left == 0 ? w2.left = chosenSaving.n1 : w2.right = chosenSaving.n1;
        cost -= chosenSaving.gain;

        // dbg("Connection status: %d(%d, %d), %d(%d, %d)\n",
        //     chosenSaving.n1, w1.left, w1.right, chosenSaving.n2, w2.left, w2.right);

        // dbg("Old cluster loads(%d, %d) = (%d ::> %d), (%d ::> %d)\n",
        //     chosenSaving.n1, chosenSaving.n2, w1.load, (*w1.otherEnd).load,
        //     w2.load, (*w2.otherEnd).load);

        const int newLoad = (w1.load + w2.load);
        w1.load = (w2.load = ((*w1.otherEnd).load = ((*w2.otherEnd).load = newLoad)));

        // Some pointer orchestration
        WayPoint *tmp = w1.otherEnd;
        (*w1.otherEnd).otherEnd = w2.otherEnd;
        (*w2.otherEnd).otherEnd = tmp;

        // dbg("    New cluster loads(%d, %d) = (%d ::> %d), (%d ::> %d)\n",
        //     chosenSaving.n1, chosenSaving.n2, w1.load, (*w1.otherEnd).load,
        //     w2.load, (*w2.otherEnd).load);

        if (w1.load > this->vCap)
            die("Cluster overload: %d\n", w1.load);

        // Remove infeasible savings - node sealed if not reaching depot
        const int toRemove1 = w1.left * w1.right > 0 ? chosenSaving.n1 : -1;
        const int toRemove2 = w2.left * w2.right > 0 ? chosenSaving.n2 : -1;

        // dbg("To remove1: %d\n", toRemove1);
        // dbg("To remove2: %d\n", toRemove2);

        const int vCap = this->vCap;
        lclSavings.erase(
            std::remove_if(
                lclSavings.begin(),
                lclSavings.end(),
                [&vCap, &waypoints, &chosenSaving, &toRemove1, &toRemove2]
                (const Savings::Saving & s)
        {
            return (s.n1 == chosenSaving.n1 && s.n2 == chosenSaving.n2) ||
                   (s.n1 == toRemove1 || s.n2 == toRemove1) ||
                   (s.n1 == toRemove2 || s.n2 == toRemove2) ||
                   (waypoints[s.n1].load + waypoints[s.n2].load > vCap) ||
                   (waypoints[s.n1].otherEnd == &waypoints[s.n2]);
        }),
        lclSavings.end());
    }

    // dbg("Estimate Cost: %.2f\n", cost);

    Ints hops;
    hops.push_back(0);

    for (int i = 1; i < N; i++)
    {
        WayPoint *wp = &waypoints[i];

        // Traverse only if not marked as visited and is a terminal node
        if (wp->left != -1 && wp->right * wp->left == 0)
        {
            hops.push_back(i);
            int prevId = i;
            int nextId = wp->left == 0 ? wp->right : wp->left;

            do
            {
                hops.push_back(nextId);

                if (nextId == 0)
                    break;

                wp = &waypoints[nextId];
                // if (wp->left == -1)
                //     die("Impossible route!\n");

                int tmp = nextId;
                nextId = (wp->left == prevId ? wp->right : wp->left);
                wp->left = -1;

                prevId = tmp;
            }
            while (true);
        }
    }

    return hops;
}

void Ants::search(Route& bestRoute)
{
    float bestScore    = std::numeric_limits<float>::max();

    #pragma omp parallel
    {
        unsigned int tseed = this->mySpec.rand_seed + omp_get_thread_num();

        do
        {
            #pragma omp for
            for (int i = 0; i < this->populationSize; i++)
            {
                // Construct a CVRP solution using the Savings based Ant System; (see 3.1)
                Ints myHops = walk(tseed);

                Route r(this->mySpec, myHops);
                // dbg("Route: %s\nCost: %.4f\n", r.genStr().c_str(), r.calcScoreSerious());

                // Improve the CVRP clustering by applying the Swap Local Search; (see 3.2)
                improveRoute(r);

                // Update the best found solution (if applicable);
                float myScore = r.calcScoreWithCache(realDists);
                #pragma omp critical
                {
                    if (myScore < bestScore)
                    {
                        bestScore = myScore;
                        bestRoute = r;
                    }
                }
            }

            // Update the pheromone matrix; (see 3.3)
        }
        while (false);
    }

}