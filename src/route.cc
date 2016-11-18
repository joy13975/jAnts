#include <numeric>
#include <algorithm>
#include <math.h>

#include "util.h"
#include "route.h"
#include "score.h"

double Route::getScoreSerious() const
{
    //use double because validator uses double... the error can cost 2% mark
    const int dim = this->mySpec->getDim();
    const int vcap = this->mySpec->getVCap();

    Node depot = mySpec->getNodes()[0];
    Node node1 = mySpec->getNodes()[this->myHops[0]];

    double score = realScore(depot, node1);
    int load = node1.z;
    for (int i = 1; i < dim - 1; i++)
    {
        const Node& prevNode = mySpec->getNodes()[this->myHops[i - 1]];
        const Node& currNode = mySpec->getNodes()[this->myHops[i]];

        //if overload on current node, go to node 1 and offload
        if (load + currNode.z > vcap)
        {
            score += realScore(prevNode, depot);

            //offload
            load = 0;
        }

        //if new vehicle, start from node 1
        const int newPrevNodeId = load == 0 ? 0 : this->myHops[i - 1];
        const Node& newPrevNode = mySpec->getNodes()[newPrevNodeId];

        score += realScore(newPrevNode, currNode);
        load += currNode.z;
    }

    //last vehicle must go back to node1
    const Node& lastNode = mySpec->getNodes()[this->myHops[dim - 2]];
    score += realScore(lastNode, depot);

    return score;
}

float Route::getScoreLazy()
{
    return std::isinf(this->myScore) ? calcScore() : this->myScore;
}

inline float Route::scoreWithFunc(float (*scoreFunc)(const Node&, const Node&)) const
{
    const int dim = this->mySpec->getDim();
    const int vcap = this->mySpec->getVCap();

    Node depot = mySpec->getNodes()[0];
    Node node1 = mySpec->getNodes()[this->myHops[0]];

    float score = scoreFunc(depot, node1);
    int load = node1.z;
    for (int i = 1; i < dim - 1; i++)
    {
        const Node& prevNode = mySpec->getNodes()[this->myHops[i - 1]];
        const Node& currNode = mySpec->getNodes()[this->myHops[i]];

        //if overload on current node, go to node 1 and offload
        if (load + currNode.z > vcap)
        {
            score += scoreFunc(prevNode, depot);

            //offload
            load = 0;
        }

        //if new vehicle, start from node 1
        const int newPrevNodeId = load == 0 ? 0 : this->myHops[i - 1];
        const Node& newPrevNode = mySpec->getNodes()[newPrevNodeId];

        score += scoreFunc(newPrevNode, currNode);
        load += currNode.z;
    }

    //last vehicle must go back to node1
    const Node& lastNode = mySpec->getNodes()[this->myHops[dim - 2]];
    score += scoreFunc(lastNode, depot);

    return score;
}

float Route::calcScore()
{
    return (this->myScore = scoreWithFunc(realScore));
}

float Route::calcFastScore()
{
    return scoreWithFunc(fastScore);
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
}

//copy from existing hops
Route::Route(const Spec& spec, const Ints& hops)
    : mySpec(&spec), myHops(hops)
{}

//initialise ascending and then randomise
Route::Route(const Spec& spec, int (*rngFunc)(int))
    : mySpec(&spec)
{
    this->myHops = genAscendHops();
    std::random_shuffle(this->myHops.begin(), this->myHops.end(), rngFunc);
}

Route& Route::operator=(Route other)
{
    std::swap(mySpec, other.mySpec);
    std::swap(myHops, other.myHops);
    std::swap(myScore, other.myScore);
    return *this;
}

const Ints Route::getHops() const
{
    return this->myHops;
}

String Route::genStr() const
{
    const int dim = this->mySpec->getDim();
    const int vcap = this->mySpec->getVCap();

    String outStr = "";
    outStr += "1->" + std::to_string(this->myHops[0] + 1);

    Node node1 = mySpec->getNodes()[this->myHops[0]];
    int load = node1.z;
    for (int i = 1; i < dim - 1; i++)
    {
        const Node& currNode = mySpec->getNodes()[this->myHops[i]];

        //if overload on current node, go to node 1 and offload
        if (load + currNode.z > vcap)
        {
            //offload
            load = 0;
            outStr += "->1\n1";
        }

        outStr += "->" + std::to_string(this->myHops[i] + 1);
        load += currNode.z;
    }

    //last vehicle must go back to node1
    outStr += "->1\n";

    return outStr;
}