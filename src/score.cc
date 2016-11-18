#include <cmath>

#include "score.h"

float score(const Node& p1, const Node& p2)
{
    //sqrt of fastScore
    return sqrt(fastScore(p1, p2));
}

float fastScore(const Node& p1, const Node& p2)
{
    //compute squared euclidean distance
    return pow(p2.x - p1.x, 2) + pow(p2.y - p1.y, 2);
}