#include <numeric>
#include <algorithm>
#include <math.h>

#include "util.h"
#include "route.h"
#include "jrng.h"

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

void Route::insertDepots()
{
    const Nodes& nodes = this->mySpec->getNodes();
    const int vcap = this->mySpec->getVCap();

    // insert depot as first hop
    this->myHops.insert(this->myHops.begin(), 0);

    int load = 0;
    for (int i = 1; i < this->myHops.size(); i++)
    {
        const Node& currNode = mySpec->getNodes()[this->myHops[i]];

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

Ints Route::genAscendHops()
{
    //initialise route - node 1 (id 0) is skipped
    Ints hops = Ints(this->mySpec->getDim() - 1);

    // Fill with increasing number from 1
    std::iota(std::begin(hops), std::end(hops), 1);

    return hops;
}

//initialise as ascending hops
Route::Route(const Spec& spec)
    : mySpec(&spec)
{
    this->myHops = genAscendHops();
    this->insertDepots();
}

//copy from existing hops
Route::Route(const Spec& spec, const Ints& hops)
    : mySpec(&spec), myHops(hops)
{
    // Assume caller has prepared hops with depots
    // this->insertDepots();
}

//initialise ascending and then randomise
Route::Route(const Spec& spec, unsigned int& seed)
    : mySpec(&spec)
{
    this->myHops = genAscendHops();
    jRNG::random_shuffle(seed, this->myHops.begin(), this->myHops.end());
    this->insertDepots();
}

Route Route::Dummy()
{
    Route d = Route();
    d.dummy = true;

    return d;
}

Route& Route::operator=(Route other)
{
    std::swap(mySpec, other.mySpec);
    std::swap(myHops, other.myHops);
    std::swap(dummy, other.dummy);
    return *this;
}

const Ints Route::getHops() const
{
    return this->myHops;
}

String Route::genStr() const
{
    const int N = this->myHops.size();

    String outStr = "1";

    const Node& node1 = mySpec->getNodes()[this->myHops[0]];
    for (int i = 1; i < N - 1; i++)
    {
        const int currId = this->myHops[i];
        outStr += "->" + std::to_string(currId + 1);
        if (currId == 0)
            outStr += "\n1";
    }

    //last vehicle must go back to node1
    outStr += "->1\n";

    return outStr;
}