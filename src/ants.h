#ifndef _ANTS_H_
#define _ANTS_H_
#include <sstream>

#include "route.h"
#include "spec.h"
#include "savings.h"

#define DEFAULT_ACO_ALPHA           5.0f    //importance of pheromone trace
#define DEFAULT_ACO_BETA            1.0f    //importance of bias
#define DEFAULT_ACO_PHEROMONE       1.0f
#define DEFAULT_ACO_PERSISTENCE     0.975f
#define DEFAULT_ACO_MIN_PHERO       0.02f
#define DEFAULT_ACO_NBHOOD_DIV      10

class Ants
{
public:
    Ants(const Spec& spec,
         const long popSize,
         const long maxStag,
         const float alpha,
         const float beta,
         const float pers,
         const float minPhero,
         const int nbhood,
         std::stringstream& dataStream);
    virtual ~Ants() {};
    void search(Route& bestRoute, const double startTime);

private:
    const Spec& mySpec;
    const long myPopSize;
    const long myMaxStag;
    const float myAlpha, myBeta, myPers, myMinPhero;
    const int myNBHood;
    std::stringstream& myStream;

    const Nodes& myNodes;
    const int myDim;
    const int myVCap;

    Cache<float> myDists;

    typedef struct Trail : public Savings::Saving
    {
        Trail(const int n1val, const int n2val, const float gainVal)
            : Saving(n1val, n2val, gainVal) {};
        float pheromone = 1.0f;
    } Trail;
    typedef std::vector<Trail> Trails;
    Trails myTrails;

    typedef struct Path
    {
        Ints hops;
        int load;
    } Path;
    typedef std::vector<Path> Paths;
    inline void applyOneExchange(Paths& paths);
    inline void applyTwoOpt(Path& path);

    inline void improvePaths(Paths& paths);
    inline Ints pathToHops(const Paths &paths);

    typedef struct WayPoint
    {
        int left, right;
        int load;
        struct WayPoint *otherEnd;
    } WayPoint;
    typedef std::vector<WayPoint> WayPoints;
    inline Paths wayPointsToPaths(WayPoints localWayPoints);
    inline WayPoints applySavings(unsigned int& seed, Trails lclTrails);
    inline Paths walk(unsigned int& seed);
};

#endif /* include guard */