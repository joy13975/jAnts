#include <cstdlib>

#include "spec.h"
#include "util.h"

Spec::Spec(const int _rand_seed)
    : rand_seed(_rand_seed)
{
    std::srand(this->rand_seed);
}

const Nodes& Spec::getNodes() const
{
    return this->nodes;
}

void Spec::setNodes(Nodes ns)
{
    if (this->nodesSet)
        die("Should never setNodes() more than once\n");

    this->nodes = ns;
    this->nodesSet = true;
}


int Spec::getDim() const
{
    return this->dim;
}

void Spec::setDim(const int val)
{
    this->dim = val;
}

int Spec::getSqDim() const
{
    return this->sqDim;
}

void Spec::setSqDim(const int val)
{
    this->sqDim = val;
}

int Spec::getVCap() const
{
    return this->vCap;
}

void Spec::setVCap(const int val)
{
    this->vCap = val;
}
