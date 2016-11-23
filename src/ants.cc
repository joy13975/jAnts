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
        for (int j = 0; j < paths[i].size(); j++)
        {
            const int node1 = paths[i][j];

            // For each node, find another path that is
            // on average closer than its current path
            float minSumDist = std::numeric_limits<float>::max();
            float minMinDist = std::numeric_limits<float>::max();
            int nearestPathId = -1, nearestNodeIndex = -1;
            for (int k = 0; k < paths.size(); k++)
            {
                float sumDist = 0.0f;
                float minDist = std::numeric_limits<float>::max();
                int minDistIndex = -1;
                for (int l = 0; l < paths[k].size(); l++)
                {
                    const float dist = this->myDists[node1][paths[k][l]];
                    sumDist += dist;
                    if (dist < minDist)
                    {
                        minDist = dist;
                        minDistIndex = l;
                    }
                }

                if (sumDist < minSumDist)
                {
                    minSumDist = sumDist;
                    nearestPathId = k;
                    minMinDist = minDist;
                    nearestNodeIndex = minDistIndex;
                }
            }

            if (nearestPathId != i)
            {
                // paths[nearestPathId] might be a better home for node1,
                // and paths[k][nearestNodeIndex] is the best candidate for swapping
                const float oldCostSum = Route(this->myNodes,
                                               paths[i],
                                               this->myVCap).
                                         calcScoreWithCache(this->myDists)
                                         +
                                         Route(this->myNodes,
                                               paths[nearestPathId],
                                               this->myVCap).
                                         calcScoreWithCache(this->myDists);
                Ints newPath1 = Ints(paths[i]), newPath2 = Ints(paths[nearestPathId]);
                newPath1[j] = newPath2[nearestNodeIndex];
                newPath2[nearestNodeIndex] = node1;
                const float newCostSum = Route(this->myNodes,
                                               newPath1,
                                               this->myVCap).
                                         calcScoreWithCache(this->myDists)
                                         +
                                         Route(this->myNodes,
                                               newPath2,
                                               this->myVCap).
                                         calcScoreWithCache(this->myDists);

                if (newCostSum < oldCostSum)
                {
                    paths[i] = newPath1;
                    paths[nearestPathId] = newPath2;
                }
            }
        }
    }
}

typedef std::vector<std::set<int>> Forest;
inline void Ants::applyKruskal(Ints& path)
{
    const int nHops = path.size() - 1;
    const int nEdges = nHops * (nHops - 1) / 2;
    Edges E;
    E.reserve(nEdges);

    for (int i = 0; i < nHops; i++)
        for (int j = i + 1; j < nHops; j++)
            E.emplace_back(path[i], path[j]);

    std::sort(E.begin(),
              E.end(),
    [this](const Int2 & e1, const Int2 & e2) {
        return this->myDists[e1.x][e1.y] < this->myDists[e2.x][e2.y];
    });

    Forest F = Forest(nHops);
    for (int i = 0; i < nHops; i++)
        F[i].insert(path[i]);

    Edges mst;
    mst.reserve(nHops - 1);
    int edgeId = -1;

    for (int i = 0; i < nEdges; i++)
    {
        const Int2& edge = E[i];
        const int nTrees = F.size();

        if (nTrees == 1)
            break;

        // Find tree ids
        int t1 = -1, t2 = -1;
        for (int j = 0; j < nTrees; j++)
        {
            if (F[j].find(edge.x) != F[j].end())
                t1 = j;
            if (F[j].find(edge.y) != F[j].end())
                t2 = j;
        }

        // If edge ends belong to different trees, merge, else ignore
        if (t1 != t2)
        {
            F[t1].insert(F[t2].begin(), F[t2].end());
            F.erase(F.begin() + t2);
            mst.push_back(edge);
            // dbg("Push edge: %d-%d\n", edge.x, edge.y);

            // Record first edge id as the one having a depot
            if (edge.x == 0) // x always smaller than y
                edgeId = mst.size() - 1;
        }
    }

    // Turn MST into path by doubling its edges and doing DFS
    const Route r1 = Route(this->myNodes, path, -1);
    const float c1 = r1.calcScoreWithCache(this->myDists);
    mst.insert(mst.end(), mst.begin(), mst.end());
    Ints newPath;
    newPath.reserve(path.size());
    newPath.push_back(0);
    int lastNode = 0, edgeWalked = 0;
    while (edgeWalked < 2 * (nHops - 1))
    {
        // dbg("lastNode: %d, edgeWalked: %d (< %d)\n", lastNode, edgeWalked, 2 * nHops);

        // Find next edge to walk
        int eId = -1, nextNode = -1;
        for (int i = 0; i < mst.size(); i++)
        {
            if (mst[i].x == lastNode)
            {
                eId = i;
                nextNode = mst[i].y;
                break;
            }
            else if (mst[i].y == lastNode)
            {
                eId = i;
                nextNode = mst[i].x;
                break;
            }
        }

        // Walk edge
        mst.erase(mst.begin() + eId);
        if (std::find(newPath.begin(), newPath.end(), nextNode) == newPath.end())
            newPath.push_back(nextNode);
        lastNode = nextNode;
        edgeWalked++;
    }
    newPath.push_back(0);

    const Route r2 = Route(this->myNodes, newPath, -1);
    const float c2 = r2.calcScoreWithCache(this->myDists);

    // dbg("%.2f vs %.2f\n", c1, c2);
    // dbg("r1: %s", Route::genStr(r1.getHops()).c_str());
    // dbg("r2: %s", Route::genStr(r2.getHops()).c_str());

    if (c2 < c1)
        path = newPath;
}

inline void Ants::improvePaths(Paths& paths)
{
    for (int i = 0; i < paths.size(); i++)
        applyKruskal(paths[i]);

    applyOneExchange(paths);
}

inline Ints Ants::pathsToHops(const Paths &paths)
{
    Ints hops;

    for (int i = 0; i < paths.size(); i++)
        hops.insert(hops.end(),
                    paths[i].begin(),
                    i < paths.size() - 1 ? paths[i].end() - 1 : paths[i].end());
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
            paths.push_back(Ints());
            Ints& hops = paths.back();

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
            cmlProbs[i] = (probSum += pow(s.gain, myAlpha) * pow(s.getPhero(), myBeta));
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
                // Construct a CVRP solution using the Savings based Ant System; (see 3.1)
                Paths paths = walk(tseed);

                // Improve the CVRP clustering by applying the Swap Local Search; (see 3.2)
                improvePaths(paths);
                Route r(myNodes, pathsToHops(paths), 0);
                // dbg("Route: %s\nCost: %.4f\n", r.genStr().c_str(), r.calcScoreSerious());

                // Update the best found solution (if applicable);
                float myScore = r.calcScoreWithCache(this->myDists);
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
                msg("itr# %5.d, best %6.4f, time %6.2f, stagnancy %4.2f%%\n",
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
                t.setPhero(
                    std::max(
                        std::max(
                            (this->myPers * t.getPhero() +
                             (1 - this->myPers) * taken[t.n1][t.n2]),
                            this->myMinPhero
                        ),
                        stagnancy / 10.0f
                    )
                );
            }
        }
        while (stagnancy < 1.0f);
    }
}
