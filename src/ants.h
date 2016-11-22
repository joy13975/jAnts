#ifndef _ANTS_H_
#define _ANTS_H_
#include <sstream>

#include "route.h"
#include "spec.h"
#include "savings.h"

#define DEFAULT_ACO_ALPHA           1.0f    //importance of pheromone trace
#define DEFAULT_ACO_BETA            1.0f    //importance of bias
#define DEFAULT_ACO_PHEROMONE       1.0f
#define DEFAULT_ACO_PERSISTENCE     0.975f
#define DEFAULT_ACO_MIN_PHERO       0.05f
#define DEFAULT_ACO_NBHOOD          4
#define DEFAULT_ACO_LAMDA           1

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
         const int lamda,
         std::stringstream& dataStream);
    virtual ~Ants() {};
    void search(Route& bestRoute, const double startTime);

private:
    const Spec& mySpec;
    const long myPopSize;
    const long myMaxStag;
    const float myAlpha, myBeta, myPers, myMinPhero;
    const int myNBHood;
    const int myLamda;
    std::stringstream& myStream;

    const Nodes& myNodes;
    const int myDim;
    const int myVCap;

    Cache<float> myDists, myPheros;
    Savings::Savings mySavings;

    typedef std::vector<Ints> Paths;
    inline void applyLamdaExchange(Ints& path);
    inline void applyKruskal(Ints& path);
    inline void applyChristofide(Ints& path);
    inline void improvePaths(Paths& paths);
    inline Ints pathsToHops(const Paths &paths);

    typedef struct WayPoint
    {
        int left, right;
        int load;
        struct WayPoint *otherEnd;
    } WayPoint;
    typedef std::vector<WayPoint> WayPoints;
    inline Paths wayPointsToPaths(WayPoints localWayPoints);
    inline WayPoints applySavings(unsigned int& seed,
                                  Savings::Savings lclSavings);
    inline Paths walk(unsigned int& seed);
};

#endif /* include guard */