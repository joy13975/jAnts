#ifndef _ROUTE_H_
#define _ROUTE_H_

#include <limits>

#include "typedefs.h"
#include "node.h"
#include "spec.h"
#include "score.h"

typedef std::vector<Int2> Edges;

class Route
{
public:
    Route(const Nodes& nodes, const int vcap);
    Route(const Nodes& nodes, const Ints hops, const int vcap);
    Route(const Nodes& nodes, const int vcap, unsigned int& seed);
    static Route Dummy();

    virtual ~Route() {};

    const Ints& getHops() const;
    const Edges& getEdges() const;
    static String genStr(const Ints& hops);
    static Edges genEdges(const Ints& hops);
    static Ints genAscendHops(const int dim);

    float calcRealScore() const;
    float calcFastScore() const;
    double calcScoreSerious() const;
    bool isDummy();

    template<typename T>
    T calcScoreWithCache(const Cache<T>& C) const
    {
        const int N = this->myHops.size();

        T score = 0.0f;
        #pragma omp simd
        for (int i = 1; i < N; i++)
            score += C[this->myHops[i - 1]][this->myHops[i]];

        return score;
    }

private:
    const Nodes *myNodes;
    Ints myHops;
    bool dummy = false;
    Edges myEdges;

    Route() : myNodes(NULL) {};
    inline void insertDepots(const int vcap);

    template<typename T>
    inline T scoreWithFunc(T (*scoreFunc)(const Node&, const Node&)) const
    {
        const int N = this->myHops.size();

        T score = 0.0f;

        #pragma omp simd
        for (int i = 1; i < N; i++)
            score += scoreFunc((*this->myNodes)[this->myHops[i - 1]],
                               (*this->myNodes)[this->myHops[i]]);

        return score;
    }
};

typedef std::vector<Route> Routes;

#endif /* include guard */