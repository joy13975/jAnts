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
           const int nbhood,
           const int lamda,
           std::stringstream& dataStream)
    : mySpec(spec), myPopSize(popSize), myMaxStag(maxStag),
      myAlpha(alpha), myBeta(beta), myPers(pers), myMinPhero(minPhero),
      myNBHood(spec.getDim() / nbhood), myLamda(lamda),
      myStream(dataStream),
      myNodes(spec.getNodes()), myDim(spec.getDim()), myVCap(spec.getVCap())
{
    this->myDists       = Score::makeScoreCache(this->myNodes, Score::real);
    this->myPheros    = makeCache(this->myDim, 1.0f);
    this->mySavings   = Savings::makeSavings(this->myDists);
}

inline void Ants::applyLamdaExchange(Ints& path)
{

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
    const Route r1 = Route(this->myNodes, path);
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

    const Route r2 = Route(this->myNodes, newPath);
    const float c2 = r2.calcScoreWithCache(this->myDists);

    // dbg("%.2f vs %.2f\n", c1, c2);
    // dbg("r1: %s", Route::genStr(r1.getHops()).c_str());
    // dbg("r2: %s", Route::genStr(r2.getHops()).c_str());

    if (c2 < c1)
        path = newPath;
}

inline void Ants::applyChristofide(Ints& path)
{
    applyKruskal(path);

    applyLamdaExchange(path);
}

inline void Ants::improvePaths(Paths& paths)
{
    #pragma omp simd
    for (int i = 0; i < paths.size(); i++)
        applyChristofide(paths[i]);
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

inline Ants::WayPoints Ants::applySavings(unsigned int& seed,
        Savings::Savings lclSavings)
{
    WayPoints wayPoints = WayPoints(this->myDim);
    for (int i = 1; i < this->myDim; i++)
    {
        wayPoints[i] = {0, 0, this->myNodes[i].z, 0};
        wayPoints[i].otherEnd = &wayPoints[i];
    }

    // Apply savings until no more feasible
    while (lclSavings.size() > 0)
    {
        // Accumulate probabilities (no need to sort)
        const int cmlProbsSize = std::min(myNBHood, (int) lclSavings.size());
        Floats cmlProbs = Floats(cmlProbsSize, 0.0f);
        float probSum = 0.0f;

        #pragma omp simd
        for (int i = 0; i < cmlProbsSize; i++)
        {
            const Savings::Saving& s = lclSavings[i];
            cmlProbs[i] = (probSum += pow(s.gain, myAlpha) * pow(this->myPheros[s.n1][s.n2], myBeta));
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

        const Savings::Saving& chosenSaving = lclSavings[chosenSId];
        // dbg("Saving %d~%d, id: %d, savings size: %d\n",
        //     chosenSaving.n1, chosenSaving.n2, chosenSId, lclSavings.size());

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
        lclSavings.erase(
            std::remove_if(
                lclSavings.begin(),
                lclSavings.end(),
                [&myVCap, &wayPoints, &chosenSaving, &toRemove1, &toRemove2]
                (const Savings::Saving & s)
        {
            return (s.n1 == chosenSaving.n1 && s.n2 == chosenSaving.n2) ||
                   (s.n1 == toRemove1 || s.n2 == toRemove1) ||
                   (s.n1 == toRemove2 || s.n2 == toRemove2) ||
                   (wayPoints[s.n1].load + wayPoints[s.n2].load > myVCap) ||
                   (wayPoints[s.n1].otherEnd == &wayPoints[s.n2]);
        }),
        lclSavings.end());
    }

    return wayPoints;
}

inline Ants::Paths Ants::walk(unsigned int& seed)
{
    WayPoints wayPoints = applySavings(seed, this->mySavings);

    return wayPointsToPaths(wayPoints);
}

void Ants::search(Route& bestRoute, const double startTime)
{
    float bestScore = std::numeric_limits<float>::max();
    float prevBestScore = bestScore;
    long stagnantCount = 0;
    int itr = 0;
    Edges bestEdges;
    Cache<bool> taken;

    msg("ACO settings: \n");
    raw_at(LOG_MESSAGE, "alpha:     %.3f\n",    this->myAlpha);
    raw_at(LOG_MESSAGE, "beta:      %.3f\n",    this->myBeta);
    raw_at(LOG_MESSAGE, "pers:      %.3f\n",    this->myPers);
    raw_at(LOG_MESSAGE, "minPhero:  %.3f\n",    this->myMinPhero);
    raw_at(LOG_MESSAGE, "nbhood:    %d\n",      this->myNBHood);
    raw_at(LOG_MESSAGE, "lamda:     %d\n",      this->myLamda);
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
                Route r(myNodes, pathsToHops(paths));
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
                msg("itr# %5.d, best %6.4f, time %6.2f, stagnant %4.2f%%\n",
                    itr,
                    bestScore,
                    (get_timestamp_us() - startTime) / 1e6,
                    (float) 100.0f * stagnantCount / myMaxStag);
                bestEdges = Edges(bestRoute.getEdges());
                taken = makeCache(this->myDim, false);
                this->myStream << itr << " " << std::fixed << std::setprecision(4) << bestScore << "\n";
            }

            // Update the pheromone matrix; (see 3.3)
            #pragma omp for simd
            for (int i = 0; i < bestEdges.size(); i++)
            {
                const Int2& edge = bestEdges[i];
                taken[edge.x][edge.y] = (taken[edge.y][edge.x] = true);
            }

            #pragma omp for simd
            for (int i = 0; i < this->myDim; i++)
                for (int j = 0; j < this->myDim; j++)
                    this->myPheros[i][j] = std::max((this->myPers * this->myPheros[i][j] +
                                                     (1 - this->myPers) * taken[i][j]),
                                                    this->myMinPhero);
        }
        while (stagnantCount < myMaxStag);
    }
}