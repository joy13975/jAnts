#ifndef _CACHE_H_
#define _CACHE_H_

template<typename T>
using CacheRow = std::vector<T>;
template<typename T>
using Cache = std::vector<CacheRow<T>>;

template<typename T>
Cache<T> makeCache(const int N, T val)
{
    Cache<T> C;
    C.reserve(N);
    for (int i = 0; i < N; i++)
    {
        CacheRow<T> R;
        R.reserve(N);

        for (int j = 0; j < N; j++)
            R.push_back(val);

        C.push_back(R);
    }

    return C;
}

#endif /* include guard */