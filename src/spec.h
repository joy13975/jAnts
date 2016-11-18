#ifndef _SPEC_H_
#define _SPEC_H_

#include "typedefs.h"
#include "node.h"

class Spec
{
public:
    Spec() {};
    virtual ~Spec() {};

    const Nodes getNodes() const;
    void setNodes(Nodes ns);
    int getDim() const;
    void setDim(const int val);
    int getSqDim() const;
    void setSqDim(const int val);
    int getVCap() const;
    void setVCap(const int val);

private:
    Nodes nodes;
    bool nodesSet = false;
    int dim, sqDim, vCap;
};

#endif /* include guard */