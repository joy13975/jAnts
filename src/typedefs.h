#ifndef _TYPEDEFS_H
#define _TYPEDEFS_H

typedef struct Node
{
    int x, y;
    int demand;
} Node;

typedef struct CVRP_Spec
{
    Node *nodes;
    int n_nodes, n_savings, capacity;
    double start_time_us;
} CVRP_Spec;

#endif /* include guard */