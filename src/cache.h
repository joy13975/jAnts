#ifndef _CACHE_H_
#define _CACHE_H_

template<typename T>
using CacheRow = std::vector<T>;
template<typename T>
using Cache = std::vector<CacheRow<T>>;

#endif /* include guard */