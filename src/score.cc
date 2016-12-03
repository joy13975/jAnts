#include <cmath>

#include "score.h"

namespace Score
{
double serious(const Node& p1, const Node& p2)
{
    return sqrt(pow((double) p2.x - (double) p1.x, 2) + pow((double) p2.y - (double) p1.y, 2));
}

float inv(const Node& p1, const Node& p2)
{
    float f = real(p1, p2);
    return f == 0 ? 0 : 1 / f;
}

float real(const Node& p1, const Node& p2)
{
    //sqrt of fastScore
    return sqrt(fast(p1, p2));
}

float fast(const Node& p1, const Node& p2)
{
    //compute squared euclidean distance
    return pow(p2.x - p1.x, 2) + pow(p2.y - p1.y, 2);
}
}