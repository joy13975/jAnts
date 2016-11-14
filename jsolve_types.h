#ifndef _JSOLVE_TYPES_H
#define _JSOLVE_TYPES_H

typedef struct node
{
    int x, y;
    int demand;
} node;

typedef struct specification
{
    node *nodes;
    int n_nodes, n_savings, capacity;
    double start_time_us;
} specification;

#endif /* include guard */