#include <numeric>
#include <algorithm>
#include <math.h>

#include "util.h"
#include "route.h"
#include "jrng.h"


const Ints& Route::getHops() const
{
    return this->myHops;
}

String Route::genStr(const Ints& hops)
{
    const int N = hops.size();

    String outStr = "1";

    for (int i = 1; i < N - 1; i++)
    {
        const int currId = hops[i];
        outStr += "->" + std::to_string(currId + 1);
        if (currId == 0)
            outStr += "\n1";
    }

    //last vehicle must go back to node1
    outStr += "->1\n";

    return outStr;
}

const Edges& Route::getEdges() const
{
    return this->myEdges;
}

Edges Route::genEdges(const Ints& hops)
{
    const int N = hops.size();
    Edges es;
    es.reserve(2 * N);

    for (int i = 1; i < N; i++)
        es.emplace_back(hops[i - 1], hops[i]);

    return es;
}

double Route::calcScoreSerious() const
{
    return scoreWithFunc(Score::serious);
}

float Route::calcRealScore() const
{
    return scoreWithFunc(Score::real);
}

float Route::calcFastScore() const
{
    return scoreWithFunc(Score::fast);
}

bool Route::isDummy()
{
    return this->dummy;
}

void Route::insertDepots(const int vcap)
{
    // insert depot as first hop
    this->myHops.insert(this->myHops.begin(), 0);

    int load = 0;
    for (int i = 1; i < this->myHops.size(); i++)
    {
        const Node& currNode = (*this->myNodes)[this->myHops[i]];

        // if overload on current node, go to node 1 and offload
        load += currNode.z;
        if (load > vcap)
        {
            load = currNode.z;
            this->myHops.insert(this->myHops.begin() + i, 0);
        }
        const int currId = this->myHops[i];
    }

    // append depot as last hop
    this->myHops.push_back(0);
}

Ints Route::genAscendHops(const int dim)
{
    //initialise route - node 1 (id 0) is skipped
    Ints hops = Ints(dim - 1);

    // Fill with increasing number from 1
    std::iota(hops.begin(), hops.end(), 1);

    return hops;
}

//initialise as ascending hops
Route::Route(const Nodes& nodes, const int vcap)
    : myNodes(&nodes)
{
    this->myHops = genAscendHops(nodes.size());
    this->insertDepots(vcap);
    this->myEdges = genEdges(this->myHops);
}

//copy from existing hops
Route::Route(const Nodes& nodes, const Ints hops, const int vcap)
    : myNodes(&nodes), myHops(hops), myEdges(genEdges(this->myHops))
{
    if (vcap > 0)
        this->insertDepots(vcap);
}

//initialise ascending and then randomise
Route::Route(const Nodes& nodes, const int vcap, unsigned int& seed)
    : myNodes(&nodes)
{
    this->myHops = genAscendHops(nodes.size());
    jRNG::random_shuffle(seed, this->myHops.begin(), this->myHops.end());
    insertDepots(vcap);
    this->myEdges = genEdges(this->myHops);
}

Route Route::Dummy()
{
    Route d = Route();
    d.dummy = true;

    return d;
}