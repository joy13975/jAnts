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
#include "config.h"

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
    bool improved = true;

    int itr = 0;
    while (improved)
    {
        improved = false;

        for (int i = 0; i < paths.size(); i++)
        {
            Path& path1 = paths[i];
            Ints& path1Hops = path1.hops;

            for (int j = 1; j < paths[i].hops.size() - 1; j++)
            {
                for (int k = 0; k < paths.size(); k++)
                {
                    if (k == i)
                        continue;

                    Path& path2 = paths[k];
                    Ints& path2Hops = path2.hops;
                    for (int node2Idx = 1; node2Idx < path2Hops.size() - 1; node2Idx++)
                    {
                        // node assignments must be in inner most loop
                        // because after one change, the same node1 is no longer
                        // in path1!!!
                        const int node1Idx = j;
                        const int node1 = path1Hops[node1Idx];
                        const int node2 = path2Hops[node2Idx];

                        int load1, load2;
                        if (((load1 =
                                    path1.load - this->myNodes[node1].z +
                                    this->myNodes[path2.hops[node2Idx]].z)
                                <= this->myVCap)
                                &&
                                ((load2 =
                                      path2.load + this->myNodes[node1].z -
                                      this->myNodes[path2.hops[node2Idx]].z)
                                 <= this->myVCap))
                        {
                            const float oldPath1Cost = path1.cost;
                            const float oldPath2Cost = path2.cost;
                            const float oldCostSum = oldPath1Cost + oldPath2Cost;

                            const float path1CostDiff = - this->myDists[path1Hops[node1Idx - 1]][node1]
                                                        - this->myDists[node1][path1Hops[node1Idx + 1]]
                                                        + this->myDists[path1Hops[node1Idx - 1]][node2]
                                                        + this->myDists[node2][path1Hops[node1Idx + 1]];
                            const float newPath1Cost = oldPath1Cost + path1CostDiff;
                            const float path2CostDiff = - this->myDists[path2Hops[node2Idx - 1]][node2]
                                                        - this->myDists[node2][path2Hops[node2Idx + 1]]
                                                        + this->myDists[path2Hops[node2Idx - 1]][node1]
                                                        + this->myDists[node1][path2Hops[node2Idx + 1]];
                            const float newPath2Cost = oldPath2Cost + path2CostDiff;
                            const float newCostSum = newPath1Cost + newPath2Cost;


                            if (newCostSum < oldCostSum)
                            {
                                Ints oldPath1Hops = path1Hops;
                                path1Hops[node1Idx] = node2;
                                path1.load = load1;
                                path1.cost = newPath1Cost;
                                path2Hops[node2Idx] = node1;
                                path2.load = load2;
                                path2.cost = newPath2Cost;
                                improved = true;
                            }
                        }
                    }
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
        float bestCost = path.cost;

        #pragma omp simd
        for (int i = 1; i < nHops - 2; i++)
        {
            for (int j = i + 1; j < nHops - 1; j++)
            {

                const float costDiff =  - this->myDists[path.hops[i - 1]][path.hops[i]]
                                        - this->myDists[path.hops[j]][path.hops[j + 1]]
                                        + this->myDists[path.hops[i - 1]][path.hops[j]]
                                        + this->myDists[path.hops[i]][path.hops[j + 1]];

                const float newCost = path.cost + costDiff;

                if (newCost < bestCost)
                {
                    std::reverse(path.hops.begin() + i, path.hops.begin() + j + 1);
                    improved = true;
                    path.cost = newCost;
                    bestCost = newCost;
                }
            }
        }
    }
}

inline void Ants::improvePaths(Paths& paths)
{
    for (Path& p : paths)
        applyTwoOpt(p);

    // applyOneExchange(paths);
}

inline Ints Ants::pathToHops(const Paths &paths)
{
    Ints hops;

    #pragma omp simd
    for (int i = 0; i < paths.size(); i++)
        hops.insert(hops.end(),
                    paths[i].hops.begin(),
                    i < paths.size() - 1 ?
                    paths[i].hops.end() - 1 : paths[i].hops.end());

    return hops;
}

inline float Ants::sumPathCosts(const Paths &paths)
{
    float cost = 0.0f;
    for (const Path& p : paths)
        cost += p.cost;
    return cost;
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
            Path newPath;
            newPath.cost = 0.0f;

            // First node is depot
            newPath.hops.push_back(0);

            // Both terminal nodes have complete loads
            newPath.load = wp->load;

            int prevId = 0;
            int nextId = i;
            do
            {
                newPath.hops.push_back(nextId);
                newPath.cost += this->myDists[prevId][nextId];

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

            paths.push_back(newPath);
        }
    }

    return paths;
}

inline Ants::WayPoints Ants::applySavings(unsigned int& seed, Trails lclTrails)
{
    WayPoints wayPoints = WayPoints(this->myDim);

    #pragma omp simd
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

        #pragma omp simd
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
    int itr = 0, nPheroAtMin = 0;
    float stagnancy, currMinPhero;
    double secElapsed = 0;
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

        while (secElapsed < MAX_SECONDS_ALLOWED)
        {
            #pragma omp for
            for (int i = 0; i < this->myPopSize; i++)
            {
                Paths paths = walk(tseed);

                improvePaths(paths);

                const float myScore = sumPathCosts(paths);

                #pragma omp critical
                {
                    if (myScore < bestScore)
                    {
                        bestScore = myScore;
                        bestRoute = Route(this->myNodes, pathToHops(paths), -1);
                    }
                }
            }

            #pragma omp single
            {
                bestEdges = Edges(bestRoute.getEdges());
                taken = makeCache(this->myDim, false);
                currMinPhero = std::numeric_limits<float>::max();
                nPheroAtMin = 0;
            }

            // Find edges taken
            #pragma omp for
            for (int i = 0; i < bestEdges.size(); i++)
            {
                const Int2& edge = bestEdges[i];
                taken[edge.x][edge.y] = (taken[edge.y][edge.x] = true);
            }

            // Update pheromones
            #pragma omp for reduction(min: currMinPhero) reduction(+: nPheroAtMin)
            for (int i = 0; i < this->myTrails.size(); i++)
            {
                Trail& t = this->myTrails[i];
                t.pheromone = std::max((this->myPers * t.pheromone +
                                        (1 - this->myPers) * taken[t.n1][t.n2]),
                                       this->myMinPhero);
                if (t.pheromone < currMinPhero)
                {
                    currMinPhero = t.pheromone;
                    nPheroAtMin = 0;
                }
                else if (fabs(t.pheromone - this->myMinPhero) < 0.01)
                {
                    nPheroAtMin++;
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
                secElapsed = (get_timestamp_us() - startTime) / 1e6;
                msg("itr %5d, best %6.4f, time %6.1f, minPhero %3.2f(%3d), stagnancy %3.1f%%\n",
                    itr,
                    bestScore,
                    secElapsed,
                    currMinPhero,
                    this->myTrails.size() - nPheroAtMin,
                    100.0f * stagnancy);
                this->myStream << itr << ", "
                               << std::fixed << std::setprecision(4) << secElapsed  << ", "
                               << " " << std::fixed << std::setprecision(4) << bestScore << "\n";


                if (nPheroAtMin == this->myTrails.size() - bestEdges.size() ||
                        stagnancy == 1.0f)
                {
                    msg("Solution converged. Reinitialising pheromones...\n");

                    #pragma omp simd
                    for (int i = 0; i < this->myTrails.size(); i++)
                        this->myTrails[i].pheromone = 1.0f;
                }
            }
        }
    }
}
