#ifndef _SAVINGS_H_
#define _SAVINGS_H_

#include <algorithm>

#include "score.h"
#include "cache.h"

#define GAIN_THRESOLD   1.0f //ignore gains below this number

namespace Savings
{

struct Saving
{
    int n1, n2;
    float gain;
    Saving(const int n1val, const int n2val, const float gainVal)
        : n1(n1val), n2(n2val), gain(gainVal) {};
    bool operator < (const Saving& sv) const
    {
        return (gain > sv.gain);
    }
};
using Savings = std::vector<Saving>;

template<typename T>
Savings makeSavings(const Cache<T>& dists)
{
    const int N = dists[0].size();

    Savings S;
    S.reserve(N * N / 2);
    for (int i = 1; i < N; i++)
        for (int j = i + 1; j < N; j++)
        {
            const float gain = dists[i][0] + dists[j][0] - dists[i][j];
            if (gain > GAIN_THRESOLD)
                S.push_back(Saving(i, j, gain));
        }

    std::sort(S.begin(), S.end());

    // for (int i = 0; i < S.size(); i++)
    // {
    //     if (S[i].n1 == 9)
    //     {
    //         dbg("[%d] %d->%d: %.2f (own=%.2f, between=%.2f)\n",
    //             i, S[i].n1, S[i].n2, S[i].gain, dists[S[i].n1][0], dists[S[i].n1][S[i].n2]);
    //     }
    // }
    // die("Here\n");
    return S;
}

}

#endif /* include guard */