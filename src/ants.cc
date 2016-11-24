#include <algorithm>
#include <vector>
#include <cmath>
#include <iomanip>
#include <set>

#include "ants.h"
#include "score.h"
#include "util.h"
#include "typedefs.h"
#include "omp.h"
#include "jrng.h"
#include "jints.h"

Ants::Ants(const Spec& spec,
           const long popSize,
           const long maxStag,
           const float alpha,
           const float beta,
           const float pers,
           const float minPhero,
           const int nbhoodDiv,
           std::stringstream& dataStream)
    : mySpec(spec), myPopSize(popSize), myMaxStag(maxStag),
      myAlpha(alpha), myBeta(beta), myPers(pers), myMinPhero(minPhero),
      myNBHood(spec.getDim() / nbhoodDiv),
      myStream(dataStream),
      myNodes(spec.getNodes()), myDim(spec.getDim()), myVCap(spec.getVCap())
{
    this->myDists  = Score::makeScoreCache(this->myNodes, Score::real);
    Savings::Savings S = Savings::makeSavings(this->myDists);
    for (int i = 0; i < S.size(); i++)
        myTrails.emplace_back(S[i].n1, S[i].n2, S[i].gain);
}

inline void Ants::applyOneExchange(Paths& paths)
{
    for (int i = 0; i < paths.size(); i++)
    {
        for (int j = 1; j < paths[i].hops.size() - 1; j++)
        {
            // For each node, find another path that is
            // on average closer than its current path
            const int node1 = paths[i].hops[j];

            // Calculate sum distance of node1 from its own path
            float ownSumDist = 0.0f;
            for (int k = 1; k < paths[i].hops.size() - 1; k++)
                ownSumDist += this->myDists[node1][paths[i].hops[k]];

            int load1, load2;
            float minSumDist = ownSumDist;
            int nearestPathId = -1, nearestNodeIndex = -1;
            for (int k = 0; k < paths.size(); k++)
            {
                if (k == i)
                    continue;

                float sumDist = 0.0f;
                float minDist = std::numeric_limits<float>::max();
                int minDistIndex = -1;
                for (int l = 1; l < paths[k].hops.size() - 1; l++)
                {
                    const float dist = this->myDists[node1][paths[k].hops[l]];

                    if ((sumDist += dist) > minSumDist)
                        break;

                    if (dist < minDist)
                    {
                        minDist = dist;
                        minDistIndex = l;
                    }
                }

                // Check load compatibility
                if ((sumDist < minSumDist)
                        &&
                        ((load1 =
                              paths[i].load - this->myNodes[node1].z +
                              this->myNodes[paths[k].hops[minDistIndex]].z)
                         <= this->myVCap)
                        &&
                        ((load2 =
                              paths[k].load + this->myNodes[node1].z -
                              this->myNodes[paths[k].hops[minDistIndex]].z)
                         <= this->myVCap))
                {
                    minSumDist = sumDist;
                    nearestPathId = k;
                    nearestNodeIndex = minDistIndex;
                }
            }

            if (nearestPathId != i && nearestPathId != -1)
            {
                const int node2 = paths[nearestPathId].hops[nearestNodeIndex];

                // paths[nearestPathId] might be a better home for node1,
                // and paths[k][nearestNodeIndex] is the best candidate for swapping
                const float oldCostSum = Route(this->myNodes,
                                               paths[i].hops,
                                               this->myVCap).
                                         calcScoreWithCache(this->myDists)
                                         +
                                         Route(this->myNodes,
                                               paths[nearestPathId].hops,
                                               this->myVCap).
                                         calcScoreWithCache(this->myDists);
                Ints newHops1 = Ints(paths[i].hops);
                Ints newHops2 = Ints(paths[nearestPathId].hops);
                newHops1[j] = node2;
                newHops2[nearestNodeIndex] = node1;
                const float newCostSum = Route(this->myNodes,
                                               newHops1,
                                               this->myVCap).
                                         calcScoreWithCache(this->myDists)
                                         +
                                         Route(this->myNodes,
                                               newHops2,
                                               this->myVCap).
                                         calcScoreWithCache(this->myDists);

                if (newCostSum < oldCostSum)
                {
                    paths[i].hops[j] = node2;
                    paths[i].load = load1;
                    paths[nearestPathId].hops[nearestNodeIndex] = node1;
                    paths[nearestPathId].load = load2;
                }
            }
        }
    }
}

inline void Ants::applyTwoOpt(Path& path)
{
    // Get tour size
    const int nHops = path.hops.size();

    // repeat until no improvement is made
    bool improved = true;
    while (improved)
    {
        improved = false;
        float bestCost = Route(this->myNodes, path.hops, -1)
                         .calcScoreWithCache(this->myDists);

        #pragma omp simd
        for (int i = 1; i < nHops - 2; i++)
        {
            for (int j = i + 1; j < nHops - 1; j++)
            {
                // in order up to i, reverse up to j, then in order
                Ints newHops = path.hops;
                std::reverse(newHops.begin() + i, newHops.begin() + j + 1);

                const float newCost = Route(this->myNodes, newHops, -1)
                                      .calcScoreWithCache(this->myDists);
                if (newCost < bestCost )
                {
                    improved = true;
                    path.hops = newHops;
                    bestCost = newCost;
                }
            }
        }
    }
}

inline void Ants::improvePaths(Paths& paths)
{
    for (int i = 0; i < paths.size(); i++)
        applyTwoOpt(paths[i]);

    applyOneExchange(paths);
}

inline Ints Ants::pathToHops(const Paths &paths)
{
    Ints hops;

    for (int i = 0; i < paths.size(); i++)
        hops.insert(hops.end(),
                    paths[i].hops.begin(),
                    i < paths.size() - 1 ?
                    paths[i].hops.end() - 1 : paths[i].hops.end());

    return hops;
}

inline Ants::Paths Ants::wayPointsToPaths(WayPoints localWayPoints)
{
    Paths paths;
    for (int i = 1; i < this->myDim; i++)
    {
        WayPoint *wp = &localWayPoints[i];

        // Traverse only if not marked as visited and is a terminal node
        if (wp->left != -1 && wp->right * wp->left == 0)
        {
            paths.push_back(Path());
            Ints& hops = paths.back().hops;

            // First node is depot
            hops.push_back(0);

            // Second node
            hops.push_back(i);
            int prevId = i;
            int nextId = wp->left == 0 ? wp->right : wp->left;

            do
            {
                hops.push_back(nextId);

                if (nextId == 0) // Last node (depot) taken care of
                    break;

                wp = &localWayPoints[nextId];
                if (wp->left == -1)
                    die("Impossible route!\n");

                const int tmp = nextId;
                nextId = (wp->left == prevId ? wp->right : wp->left);
                prevId = tmp;
                wp->left = -1;
            }
            while (true);

            paths.back().load = wp->load;
        }
    }

    return paths;
}

inline Ants::WayPoints Ants::applySavings(unsigned int& seed, Trails lclTrails)
{
    WayPoints wayPoints = WayPoints(this->myDim);
    for (int i = 1; i < this->myDim; i++)
    {
        wayPoints[i] = {0, 0, this->myNodes[i].z, 0};
        wayPoints[i].otherEnd = &wayPoints[i];
    }

    // Apply savings until no more feasible
    while (lclTrails.size() > 0)
    {
        // Accumulate probabilities (no need to sort)
        const int cmlProbsSize = std::min(myNBHood, (int) lclTrails.size());
        Floats cmlProbs = Floats(cmlProbsSize, 0.0f);
        float probSum = 0.0f;

        for (int i = 0; i < cmlProbsSize; i++)
        {
            const Trail& s = lclTrails[i];
            cmlProbs[i] = (probSum += pow(s.gain, myAlpha) * pow(s.pheromone, myBeta));
        }

        // Roll dice and find corresponding id
        const float dice = jRNG::frand(seed) * probSum;
        int chosenSId = -1;
        for (int i = 0; i < cmlProbsSize; i++)
        {
            if (dice <= cmlProbs[i])
            {
                chosenSId = i;
                break;
            }
        }

        if (chosenSId == -1)
            die("Could not find saving ID! cmlProbsSize=%d, dice=%.2f, probSum=%.2f\n",
                cmlProbsSize, dice, probSum);

        const Savings::Saving& chosenSaving = lclTrails[chosenSId];
        // dbg("Saving %d~%d, id: %d, savings size: %d\n",
        //     chosenSaving.n1, chosenSaving.n2, chosenSId, lclTrails.size());

        WayPoint &w1 = wayPoints[chosenSaving.n1], &w2 = wayPoints[chosenSaving.n2];

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

        if (w1.load > this->myVCap)
            die("Cluster overload: %d\n", w1.load);

        // Remove infeasible savings - node sealed if not reaching depot
        const int toRemove1 = w1.left * w1.right > 0 ? chosenSaving.n1 : -1;
        const int toRemove2 = w2.left * w2.right > 0 ? chosenSaving.n2 : -1;

        // dbg("To remove1: %d\n", toRemove1);
        // dbg("To remove2: %d\n", toRemove2);

        const int myVCap = this->myVCap;
        lclTrails.erase(
            std::remove_if(
                lclTrails.begin(),
                lclTrails.end(),
                [&myVCap, &wayPoints, &chosenSaving, &toRemove1, &toRemove2]
                (const Savings::Saving & s)
        {
            return (s.n1 == chosenSaving.n1 && s.n2 == chosenSaving.n2) ||
                   (s.n1 == toRemove1 || s.n2 == toRemove1) ||
                   (s.n1 == toRemove2 || s.n2 == toRemove2) ||
                   (wayPoints[s.n1].load + wayPoints[s.n2].load > myVCap) ||
                   (wayPoints[s.n1].otherEnd == &wayPoints[s.n2]);
        }),
        lclTrails.end());
    }

    return wayPoints;
}

inline Ants::Paths Ants::walk(unsigned int& seed)
{
    WayPoints wayPoints = applySavings(seed, this->myTrails);

    return wayPointsToPaths(wayPoints);
}

void Ants::search(Route& bestRoute, const double startTime)
{
    float bestScore = std::numeric_limits<float>::max();
    float prevBestScore = bestScore;
    long stagnantCount = 0;
    int itr = 0;
    float stagnancy;
    Edges bestEdges;
    Cache<bool> taken;

    msg("ACO settings: \n");
    raw_at(LOG_MESSAGE, "alpha:     %.3f\n",    this->myAlpha);
    raw_at(LOG_MESSAGE, "beta:      %.3f\n",    this->myBeta);
    raw_at(LOG_MESSAGE, "pers:      %.3f\n",    this->myPers);
    raw_at(LOG_MESSAGE, "minPhero:  %.3f\n",    this->myMinPhero);
    raw_at(LOG_MESSAGE, "nbhood:    %d\n",      this->myNBHood);
    raw_at(LOG_MESSAGE, "popSize:   %ld\n",     this->myPopSize);
    raw_at(LOG_MESSAGE, "maxStag:   %ld\n",     this->myMaxStag);

    #pragma omp parallel
    {
        unsigned int tseed = this->mySpec.rand_seed + omp_get_thread_num();

        do
        {
            #pragma omp for
            for (int i = 0; i < this->myPopSize; i++)
            {
                Paths paths = walk(tseed);

                improvePaths(paths);

                const Route r(myNodes, pathToHops(paths), 0);
                const float myScore = r.calcScoreWithCache(this->myDists);
                #pragma omp critical
                {
                    if (myScore < bestScore)
                    {
                        bestScore = myScore;
                        bestRoute = r;
                    }
                }
            }

            #pragma omp single
            {
                if (bestScore == prevBestScore)
                    stagnantCount++;
                else
                    stagnantCount = 0;
                prevBestScore = bestScore;

                itr++;
                stagnancy = (float) stagnantCount / myMaxStag;
                msg("itr# %5.d, best %6.4f, time %6.2f, stagnancy %4.1f%%\n",
                    itr,
                    bestScore,
                    (get_timestamp_us() - startTime) / 1e6,
                    100.0f * stagnancy);
                bestEdges = Edges(bestRoute.getEdges());
                taken = makeCache(this->myDim, false);
                this->myStream << itr << " " << std::fixed << std::setprecision(4) << bestScore << "\n";
            }

            // Find edges taken
            #pragma omp for
            for (int i = 0; i < bestEdges.size(); i++)
            {
                const Int2& edge = bestEdges[i];
                taken[edge.x][edge.y] = (taken[edge.y][edge.x] = true);
            }

            // Update pheromones
            #pragma omp for
            for (int i = 0; i < this->myTrails.size(); i++)
            {
                Trail& t = this->myTrails[i];
                t.pheromone =
                    std::max(
                        std::max(
                            (this->myPers * t.pheromone +
                             (1 - this->myPers) * taken[t.n1][t.n2]),
                            this->myMinPhero
                        ),
                        stagnancy / 10.0f
                    );
            }
        }
        while (stagnancy < 1.0f);
    }
}
