#ifndef _ROUTE_H_
#define _ROUTE_H_

#include <limits>

#include "typedefs.h"
#include "node.h"
#include "spec.h"
#include "score.h"

class Route
{
public:
    Route(const Spec& spec);
    Route(const Spec& spec, const Ints& hops);
    Route(const Spec& spec, unsigned int& seed);
    static Route Dummy();

    virtual ~Route() {};
    Route& operator=(Route other);

    const Ints getHops() const;
    String genStr() const;
    float calcRealScore() const;
    float calcFastScore() const;
    double calcScoreSerious() const;
    bool isDummy();

    template<typename T>
    T calcScoreWithCache(const Cache<T> C) const
    {
        const int N = this->myHops.size();

        T score = 0.0f;

        #pragma omp simd
        for (int i = 1; i < N; i++)
            score += C[this->myHops[i - 1]][this->myHops[i]];

        return score;
    }

private:
    const Spec *mySpec;
    Ints myHops;
    bool dummy = false;

    Route() {};
    inline Ints genAscendHops();
    void insertDepots();

    template<typename T>
    inline T scoreWithFunc(T (*scoreFunc)(const Node&, const Node&)) const
    {
        const int N = this->myHops.size();

        T score = 0.0f;
        #pragma omp simd
        for (int i = 1; i < N; i++)
            score += scoreFunc(mySpec->getNodes()[this->myHops[i - 1]],
                               mySpec->getNodes()[this->myHops[i]]);

        return score;
    }
};

typedef std::vector<Route> Routes;

#endif /* include guard */