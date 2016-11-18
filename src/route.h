#ifndef _ROUTE_H_
#define _ROUTE_H_

#include <limits>

#include "typedefs.h"
#include "node.h"
#include "spec.h"

class Route
{
public:
    Route(const Spec& spec);
    Route(const Spec& spec, const Ints& hops);
    Route(const Spec& spec, int (*rngFunc)(int));
    virtual ~Route() {};
    Route& operator=(Route other);

    const Ints getHops() const;
    String genStr() const;
    float calcScore();
    float calcFastScore();
    float getScoreLazy();
    double getScoreSerious() const;

private:
    const Spec *mySpec;
    Ints myHops;
    float myScore = std::numeric_limits<float>::infinity();

    inline float scoreWithFunc(float (*scoreFunc)(const Node&, const Node&)) const;
    inline Ints genAscendHops();
};

#endif /* include guard */