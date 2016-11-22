#ifndef _SCORE_H_
#define _SCORE_H_

#include "jints.h"
#include "node.h"
#include "cache.h"
#include "util.h"

namespace Score
{

double serious(const Node& p1, const Node& p2);
float real(const Node& p1, const Node& p2);
float inv(const Node& p1, const Node& p2);
float fast(const Node& p1, const Node& p2);

template<typename T>
Cache<T> makeScoreCache(const Nodes& nodes, T (*scoreFunc)(const Node&, const Node&))
{
    const int N = nodes.size();

    Cache<T> C;
    C.reserve(N);
    for (int i = 0; i < N; i++)
    {
        CacheRow<T> R;
        R.reserve(N);

        for (int j = 0; j < N; j++)
            R.push_back(scoreFunc(nodes[i], nodes[j]));

        C.push_back(R);
    }

    return C;
}

};


#endif /* include guard */