#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "config.h"
#include "omp.h"
#include "ga.h"
#include "util.h"
#include "tsp_test.h"

#define DEATH_TIME                  (30 * 60 * 1e6) //30 minutes in microseconds
#define DEFAULT_RAND_SEED           0xdeadbeef

#if TSP_MODE == 1
#define DEFAULT_POP_SIZE            10000
#else
#define DEFAULT_POP_SIZE            100
#endif

#define PROBABILITY_PRECISION       100000

//TSP specific
#define DEFAULT_SURVIVAL_RATE       0.003f
#define DEFAULT_NEWCOMER_RATE       0.001f
#define DEFAULT_MUTATION_RATE       0.004f
#define DEFAULT_CONVERGE_FACTOR     200

//CVRP specific
#define DEFAULT_APPLY_RATE_UB       0.2f
#define DEFAULT_APPLY_RATE_LB       0.05f

unsigned rand_seed                  = DEFAULT_RAND_SEED;
long pop_size                       = DEFAULT_POP_SIZE;
float survival_rate                 = DEFAULT_SURVIVAL_RATE;
long n_survivors                    = DEFAULT_POP_SIZE * DEFAULT_SURVIVAL_RATE;
float newcomer_rate                 = DEFAULT_NEWCOMER_RATE;
long n_newcomers                    = DEFAULT_POP_SIZE * DEFAULT_NEWCOMER_RATE;
float mutation_rate                 = DEFAULT_MUTATION_RATE;
int converge_factor                 = DEFAULT_CONVERGE_FACTOR;
float apply_rate_ub                 = DEFAULT_APPLY_RATE_UB;
float apply_rate_lb                 = DEFAULT_APPLY_RATE_LB;

bool initialised                    = false;
specification *spec;

typedef struct route
{
    int *hops;
    float *edge_lengths;
    float fitness;
} route;
route *route_pop;

typedef struct cw_saving
{
    int src, dst;
    float saving;
} cw_saving;

typedef struct
{
    int **adjm;
    float cost;
} cvrp_state;

float **distm;
cw_saving *savings;
int curr_savings_idx = 0;
cvrp_state *curr_cvrp;


//function declarations
inline float monte_carlo_sim(cvrp_state *cvrp, unsigned *t_seed_ptr);
inline void calc_cvrp_cost(cvrp_state *cvrp, float *cost_ptr, int *vehicles_ptr, bool print);
inline void clone_cvrp(cvrp_state *src, cvrp_state **dst_ptr);
inline void use_arc(cvrp_state *cvrp, int src, int dst);
inline void accumulate_demand(cvrp_state *cvrp, int src, int *demand_ptr, int *end_node_ptr);
inline bool is_saving_feasible(cvrp_state *cvrp, int idx);
void destroy_cvrp(cvrp_state *cvrp);
void run_cvrp();

inline float euclidean(node *n0, node *n1);
inline float calc_route_cost(route *c);
inline void mutate_route(unsigned *t_seed_ptr, route *c);
inline void gen_random_route(unsigned *t_seed_ptr, route *c);
void sort_routes(const int l, const int r);
void run_tsp();
void run_ga();

void print_best_solution();
double check_current_time();

bool is_ga_initialised();
void init_cvrp();
void sort_savings(const int l, const int r);
void init_tsp();
void init_ga(specification *_spec);

void set_rand_seed(unsigned value);
void set_pop_size(long value);
void set_survival_rate(float value);
void set_newcomer_rate(float value);
void set_mutation_rate(float value);
void set_converge_factor(int value);
void set_apply_rate_ub(float value);
void set_apply_rate_lb(float value);

//function definitions
inline float monte_carlo_sim(cvrp_state *cvrp, unsigned *t_seed_ptr)
{
    //determine saving apply rate
    const float prob_range = apply_rate_ub - apply_rate_lb;
    const float apply_rate = apply_rate_lb +
                             (prob_range *
                              (rand_r(t_seed_ptr) % (PROBABILITY_PRECISION + 1)) /
                              PROBABILITY_PRECISION);

    long t_curr_savings_idx = curr_savings_idx + 1;
    while (true)
    {

        //do some checks
        if (savings[t_curr_savings_idx].saving <= 0)
            break;

        while (t_curr_savings_idx < spec->n_savings &&
                !is_saving_feasible(cvrp, t_curr_savings_idx))
            t_curr_savings_idx++;

        if (t_curr_savings_idx >= spec->n_savings - 1)
            break;

        bool apply_saving = (rand_r(t_seed_ptr) % (PROBABILITY_PRECISION + 1)) <
                            (apply_rate * PROBABILITY_PRECISION);
        if (apply_saving)
        {
            cw_saving cws = savings[t_curr_savings_idx];
            use_arc(cvrp, cws.src, cws.dst);
        }

        // #pragma omp barrier
        // if (t_curr_savings_idx > 3)die("");
        // wrn("Got here: %d (%.2f, %.2f <- %.2f, %d)\n", t_curr_savings_idx,
        //     (float) (rand_r(t_seed_ptr) % (PROBABILITY_PRECISION + 1)),
        //     (float) (apply_rate * PROBABILITY_PRECISION),
        //     apply_rate,
        //     apply_saving);

        t_curr_savings_idx++;
    }

    float cost;
    calc_cvrp_cost(cvrp, &cost, NULL, false);
    return cost;
}

inline void calc_cvrp_cost(cvrp_state *cvrp, float *cost_ptr, int *vehicles_ptr, bool print)
{
    float cost                      = 0.0f;
    int vehicles                    = 0;
    bool traversed[spec->n_nodes];
    memset(traversed, 0, spec->n_nodes * sizeof(bool));

    if (print)
        ERASE_LINE();

    //traverse all routes
    for (int i = 1; i < spec->n_nodes; i++)
    {
        if (traversed[i])
            continue;

        const int idx0 = cvrp->adjm[i][0] ? (cvrp->adjm[i][1] ? -1 : 1) : 0;
        prf("Starting #%d, idx0=%d\n", i, idx0);
        if (idx0 != -1)
        {
            vehicles++;
            int v_demand = 0;

            if (print)
                raw("%d ", 0);

            int prev = 0;
            int curr = i;

            while (curr != prev && curr)
            {
                prf("prev: %d, curr: %d, cost: %.2f\n", prev, curr, cost);
                if (curr && traversed[curr])
                {
                    raw("\n");
                    die("Messed up route! #%d -> #%d\n", prev, curr);
                }

                v_demand += spec->nodes[curr].demand;
                cost += distm[prev][curr];
                traversed[curr] |= true;

                if (print)
                    raw("%d ", curr);

                int tmp = curr;
                curr = cvrp->adjm[curr][0] == prev ? cvrp->adjm[curr][1] : cvrp->adjm[curr][0];
                prev = tmp;
            }

            //final x -> 0 arc
            prf("prev: %d, curr: %d, cost: %.2f\n", prev, curr, cost);

            v_demand += spec->nodes[curr].demand;
            cost += distm[prev][curr];

            if (print)
                raw("%d ", 0);

            if (v_demand > spec->capacity)
                die("Route has excess demand!\n");
        }
    }

    if (print)
        raw("\n");

    // for (int i = 0; i < spec->n_nodes; i++)
    // dbg("#%d: %d and %d\n", i, cvrp->adjm[i][0], cvrp->adjm[i][1]);

    if (cost_ptr)
        *cost_ptr       = cost;
    if (vehicles_ptr)
        *vehicles_ptr   = vehicles;
}

inline void clone_cvrp(cvrp_state *src, cvrp_state **dst_ptr)
{
    cvrp_state *dst = calloc(1, sizeof(*dst));
    dst->cost = src->cost;

    dst->adjm = calloc(spec->n_nodes, sizeof(*(dst->adjm)));
    for (int i = 0; i < spec->n_nodes; i++)
    {
        dst->adjm[i] = calloc(spec->n_nodes, sizeof(**(dst->adjm)));
        for (int j = 0; j < 2; j++)
            dst->adjm[i][j] = src->adjm[i][j];
    }

    *dst_ptr = dst;
}

inline void use_arc(cvrp_state *cvrp, int src, int dst)
{
    float cost_change = 0;
    const int src_0idx = cvrp->adjm[src][0] ? (cvrp->adjm[src][1] ? -1 : 1) : 0;
    const int dst_0idx = cvrp->adjm[dst][0] ? (cvrp->adjm[dst][1] ? -1 : 1) : 0;

    //withdraw arcs only when not initialising
    if (src != 0 && dst != 0)
    {
        if (src_0idx == -1)
            die("Attempting #%d -> #%d which has links to #%d and #%d but not #0\n",
                src, dst, cvrp->adjm[src][0], cvrp->adjm[src][1]);

        if (dst_0idx == -1)
            die("Attempting #%d <- #%d which has links to #%d and #%d but not #0\n",
                dst, src, cvrp->adjm[dst][0], cvrp->adjm[dst][1]);

        cost_change -= distm[src][0];
        cost_change -= distm[dst][0];
    }

    //adopt new arc from src
    cvrp->adjm[src][src_0idx] = dst;
    cvrp->adjm[dst][dst_0idx] = src;
    cost_change += distm[src][dst];

    //last, update cost to cvrp
    cvrp->cost += cost_change;

    prf("Using arc: #%d -> #%d : %.2f\n", src, dst, cvrp->cost);
}

inline void accumulate_demand(cvrp_state *cvrp, int src, int *demand_ptr, int *end_node_ptr)
{
    const int src_other = cvrp->adjm[src][0] ? cvrp->adjm[src][0] : cvrp->adjm[src][1];
    prf("Accumulate from #%d... src_other = #%d\n", src, src_other);

    int total_demand = spec->nodes[src].demand;
    int prev = src;
    int curr = src_other;
    while (curr != prev && curr)
    {
        total_demand += spec->nodes[curr].demand;
        int tmp = curr;
        curr = cvrp->adjm[curr][0] == prev ? cvrp->adjm[curr][1] : cvrp->adjm[curr][0];
        prev = tmp;
        prf("Accumulate: #%d -> #%d\n", prev, curr);
    }

    prf("Accumulate end at #%d -> #%d (double #0? %d)\n",
        prev, curr, !cvrp->adjm[src][0] && !cvrp->adjm[src][1]);

    *demand_ptr += total_demand;
    *end_node_ptr = prev;
}

inline bool is_saving_feasible(cvrp_state *cvrp, int idx)
{
    const int src = savings[idx].src, dst = savings[idx].dst;
    const int src_0idx = cvrp->adjm[src][0] ? (cvrp->adjm[src][1] ? -1 : 1) : 0;
    const int dst_0idx = cvrp->adjm[dst][0] ? (cvrp->adjm[dst][1] ? -1 : 1) : 0;

    //if either has no edge to #0 then it's not possible
    if (src_0idx == -1 || dst_0idx == -1)
        return false;

    const int src_other = cvrp->adjm[src][src_0idx ^ 0x1U];
    const int dst_other = cvrp->adjm[dst][src_0idx ^ 0x1U];

    //if they are already linked and are not then no
    if (src_other == dst || dst_other == src)
    {
        if (!(src_other == dst && dst_other == src))
            die("Node #%d is connect to #%d but not the other way round!?\n", src, dst);
        return false;
    }

    //now check demand
    int total_demand = 0, end_node[2] = { 0 };
    accumulate_demand(cvrp, src, &total_demand, &(end_node[0]));
    accumulate_demand(cvrp, dst, &total_demand, &(end_node[1]));

    //no snake eats its own tail!
    if (end_node[0] == dst || end_node[1] == src)
    {
        if (!(end_node[0] == dst && end_node[1] == src))
            die("Messed up route!\n");
        return false;
    }

    return total_demand < spec->capacity;
}

void destroy_cvrp(cvrp_state *cvrp)
{
    for (int i = 0; i < spec->n_nodes; i++)
        free(cvrp->adjm[i]);
    free(cvrp->adjm);
    free(cvrp);
}

void run_cvrp()
{
    if (apply_rate_ub < apply_rate_lb)
        die("Apply rate upper bound > apply rate lower bound!\n");

    const int print_lines = 4;
    bool should_break = false;
    float sum_costs_nsa, sum_costs_sa;
    bool apply_saving;
    long step = 0;

    #pragma omp parallel
    {
        const int thread_num = omp_get_num_threads();
        unsigned t_seed = rand_seed + omp_get_thread_num();
        #pragma omp single
        msg("Running on %d threads\n", thread_num);

        while (true)
        {
            #pragma omp single
            {
                //do some checks
                if (savings[curr_savings_idx].saving <= 0)
                {
                    wrn("Algorithm stopping due to a lack of positive savings at #%d\n",
                    curr_savings_idx);
                    should_break = true;
                }
                else
                {
                    while (curr_savings_idx < spec->n_savings &&
                    !is_saving_feasible(curr_cvrp, curr_savings_idx))
                        curr_savings_idx++;

                    if (curr_savings_idx >= spec->n_savings - 1)
                    {
                        wrn("Algorithm stopping due to a lack of feasible savings at #%d\n",
                        curr_savings_idx);
                        should_break = true;
                    }
                    else
                    {
                        if (curr_savings_idx > 0)
                            MV_CURSOR_UP(print_lines);

                        //do the prints
                        cw_saving s = savings[curr_savings_idx];
                        ERASE_LINE(); raw("Next feasible saving #%d: %d -> %d ... %.2f\n",
                        curr_savings_idx, s.src, s.dst, s.saving);
                    }
                }

                sum_costs_nsa   = 0;
                sum_costs_sa    = 0;
            }

            if (should_break)
                break;

            //create new cvrp (per thread) and apply saving
            cvrp_state *nsa_cvrp, *sa_cvrp;
            clone_cvrp(curr_cvrp, &nsa_cvrp);
            clone_cvrp(curr_cvrp, &sa_cvrp);

            cw_saving cws = savings[curr_savings_idx];
            use_arc(sa_cvrp, cws.src, cws.dst);

            //do monte carlo simulation for original cvrp
            #pragma omp for reduction(+:sum_costs_nsa)
            for (int i = 0; i < pop_size; i++)
                sum_costs_nsa += monte_carlo_sim(nsa_cvrp, &t_seed);


            //do monte carlo simulation for saving-applied cvrp
            #pragma omp for reduction(+:sum_costs_sa)
            for (int i = 0; i < pop_size; i++)
                sum_costs_sa += monte_carlo_sim(sa_cvrp, &t_seed);

            #pragma omp single
            {
                apply_saving = sum_costs_sa < sum_costs_nsa;
                if (apply_saving)
                    use_arc(curr_cvrp, cws.src, cws.dst);

                ERASE_LINE(); raw("Simulation result: %s\n", apply_saving ? "apply" : "skip");
                ERASE_LINE(); raw("Cost comparison: %.2f%%\n", 100 * sum_costs_sa / sum_costs_nsa);
                ERASE_LINE(); raw("Current cost: %.2f\n", curr_cvrp->cost);
                curr_savings_idx++;
            }

            destroy_cvrp(sa_cvrp);
            destroy_cvrp(nsa_cvrp);

            #pragma omp single
            msg("Step #%d\n", step++);
            // nsleep(1e9);
        }
    }

    print_best_solution();
    destroy_cvrp(curr_cvrp);
}



inline float euclidean(node *n0, node *n1)
{
    return sqrt(pow(n0->x - n1->x, 2) + pow(n0->y - n1->y, 2));
}

inline float calc_route_cost(route *c)
{
    int *hops = c->hops;
    float cost = 0.0f;

    for (int i = 1; i < spec->n_nodes; i++)
        cost += distm[hops[i] - 1][hops[i - 1] - 1];

    cost += distm[hops[0] - 1][hops[spec->n_nodes - 1] - 1];

    return cost;
}

inline void mutate_route(unsigned *t_seed_ptr, route *c)
{
    for (int i = 0; i <= spec->n_nodes; i++)
    {
        if ((rand_r(t_seed_ptr) % (PROBABILITY_PRECISION + 1)) >
                (mutation_rate * PROBABILITY_PRECISION))
            continue;

        int pair[2];

        // distance based bias
        // const int rand_vals[2] = {
        //     rand_r(t_seed_ptr) % ((int) c->fitness),
        //     rand_r(t_seed_ptr) % ((int) c->fitness)
        // };

        // float acc = 0;
        // for (int i = 0; i < spec->n_nodes; i++)
        //     if ((acc += c->edge_lengths[i]) >= rand_vals[0])
        //     {
        //         pair[0] = i;
        //         break;
        //     }
        // acc = 0;
        // for (int i = 0; i < spec->n_nodes; i++)
        //     if ((acc += c->edge_lengths[i]) >= rand_vals[1])
        //     {
        //         pair[1] = i;
        //         break;
        //     }

        // if (pair[0] == pair[1])
        //     if (pair[1] + 1 < spec->n_nodes)
        //         pair[1]++;
        //     else
        //         pair[0]--;

        // raw("%.2f (%d %d) - %d %d\n",
        //     c->fitness, rand_vals[0], rand_vals[1], pair[0], pair[1]);

        //random - no heuristic
        pair[0] = rand_r(t_seed_ptr) % spec->n_nodes;
        pair[1] = rand_r(t_seed_ptr) % spec->n_nodes;

        //distribution biased
        // pair[0] = rand_r(t_seed_ptr) % (spec->n_nodes - 1);
        // pair[1] = pair[0] + 1;

        int tmp = c->hops[pair[0]];
        c->hops[pair[0]] = c->hops[pair[1]];
        c->hops[pair[1]] = tmp;
    }
}

inline void gen_random_route(unsigned *t_seed_ptr, route *c)
{
    memset(c->hops, 0, spec->n_nodes * sizeof(*(c->hops)));

    //randomly fill index untill full
    c->hops[0] = 1;
    for (int node_id = 2; node_id <= spec->n_nodes; node_id++)
    {
        //find suitable index to fill next node id
        int fill_idx = 0;
        do
        {
            fill_idx = rand_r(t_seed_ptr) % spec->n_nodes;
        }
        while (c->hops[fill_idx]);

        c->hops[fill_idx] = node_id;
    }
}

//simple in-place quick sort ascending
void sort_routes(const int l, const int r)
{
    if (r - l < 1)
        return;

    //pick pivot
    float pivot = route_pop[l].fitness;
    int nl = l, nr = r;

    while (nl <= nr) {

        //find inconsistent pair
        while (nl < r && route_pop[nl].fitness < pivot)
            nl++;

        while (nr > l && route_pop[nr].fitness > pivot)
            nr--;

        if (nl <= nr) {
            if (route_pop[nl].fitness != route_pop[nr].fitness)
            {
                //swap
                route tmp = route_pop[nl];
                route_pop[nl] = route_pop[nr];
                route_pop[nr] = tmp;
            }

            nl++;
            nr--;
        }
    }

    //sort partitions
    sort_routes(l, nr);
    sort_routes(nl, r);
}

void run_tsp()
{
    long generation = 1;
    int stale_count = 0, max_staleness = -999;
    float curr_best_cost, last_best_cost = INFINITY;
    const int pop_print_cap = ITERATION_PRINT_SAMPLES;
    const int gene_print_cap = ITERATION_PRINT_SAMPLES;
    const int print_lines = min(pop_print_cap, pop_size) + 5;

    #pragma omp parallel
    {
        unsigned t_seed = rand_seed + omp_get_thread_num();

        #pragma omp single
        {
            msg("Running GA with %d threads\n", omp_get_num_threads());
        }

        while (1)
        {
            //compute sum of sqrt (SoS) cost
            #pragma omp for simd
            for (int i = 0; i < pop_size; i++)
                route_pop[i].fitness = calc_route_cost(&(route_pop[i]));

            #pragma omp single
            {
                //pick out good parents (low squared cost)
                sort_routes(0, pop_size - 1);

                ERASE_LINE(); raw("Generation #%d\n", generation);
                for (int i = 0; i < pop_size && i < pop_print_cap; i++)
                {
                    ERASE_LINE(); raw("Top #%2d: ", i + 1);
                    for (int j = 0; j < spec->n_nodes && j < gene_print_cap; j++)
                        raw("%d ", route_pop[i].hops[j]);
                    float cost = calc_route_cost(&(route_pop[i]));
                    if (i == 0)
                        curr_best_cost = cost;
                    raw(": %.2f\n", cost);
                }

                ERASE_LINE(); raw("Best cost: %.2f\n", curr_best_cost);
                ERASE_LINE(); raw("Worst cost: %.2f\n", calc_route_cost(&(route_pop[pop_size - 1])));
                if (max_staleness < stale_count)
                    max_staleness = stale_count;
                ERASE_LINE(); raw("Current staleness: %d\n", max_staleness);
                ERASE_LINE(); raw("Current time: %.2f s\n", check_current_time(spec->start_time_us));
            }

            //survivors are preserved and duplicated
            if (n_survivors > 0)
            {
                #pragma omp for simd
                for (int i = n_survivors; i < pop_size; i++)
                {
                    memcpy(route_pop[i].hops,
                           route_pop[i % n_survivors].hops,
                           spec->n_nodes * sizeof(*(route_pop[i].hops)));
                    memcpy(route_pop[i].edge_lengths,
                           route_pop[i % n_survivors].edge_lengths,
                           spec->n_nodes * sizeof(float));
                    route_pop[i].fitness = route_pop[i % n_survivors].fitness;
                }
            }

            if (n_newcomers > 0)
            {
                //generate newcomers - totally random new routes
                #pragma omp for simd
                for (int i = n_survivors; i < n_survivors + n_newcomers; i++)
                    gen_random_route(&t_seed, &(route_pop[i]));
            }

            //mutate the rest
            #pragma omp for simd
            for (int i = n_survivors + n_newcomers; i < pop_size; i++)
                mutate_route(&t_seed, &(route_pop[i]));

            #pragma omp single
            {
                generation++;
                if (curr_best_cost == last_best_cost)
                    stale_count++;
                else
                    stale_count = 0;
                last_best_cost = curr_best_cost;
                // nsleep(5e8);
            }

            if (stale_count > converge_factor)
            {
                break;
            }
            else
            {
                #pragma omp single
                {
                    MV_CURSOR_UP(print_lines);
                    // CLEAR_TERM();
                }
            }

            #pragma omp barrier
        }
    }

    msg("GA has converged\n");
    print_best_solution();
}

void run_ga()
{
    //convert rates into integer numbers
    n_survivors  = pop_size * survival_rate;
    n_newcomers  = pop_size * newcomer_rate;

    if (n_survivors + n_newcomers > pop_size)
    {
        die("Too many survivors(%ld) + newcomers(%ld) > population(%ld)\n",
            n_survivors, n_newcomers, pop_size);
    }
    else
    {
#if TSP_MODE == 1
        msg("Survival rate: %.2f%% (%ld)\n", 100 * survival_rate, n_survivors);
        msg("Newcomer rate: %.2f%% (%ld)\n", 100 * newcomer_rate, n_newcomers);
        msg("Mutation rate: %.2f%% (~%.2f)\n", 100 * mutation_rate, spec->n_nodes * mutation_rate);
        msg("Converge factor: %d\n", converge_factor);
        msg("Population size: %ld\n", pop_size);
#else
        msg("Saving apply rate: %.2f%% ~ %.2f%%\n", 100 * apply_rate_lb, 100 * apply_rate_ub);
#endif
    }

#if TSP_MODE == 1
    run_tsp();
#else
    run_cvrp();
#endif
    free(distm);
}



double check_current_time()
{
    double t = (get_timestamp_us() - spec->start_time_us);
    if (t >= (DEATH_TIME - (10 * 1e6)))
    {
        raw("\n\nApproching runtime limite (%.2f min) - stopping GA\n",
            (double) DEATH_TIME / 60 / 1e6);
        print_best_solution();
        exit(0);
    }
    else
    {
        return t / 1e6; //seconds
    }
}

void print_best_solution()
{
    float best_cost;

#if TSP_MODE == 1
    best_cost = calc_route_cost(&(route_pop[0]));
    ERASE_LINE(); raw("Best route has cost %.2f: \n", best_cost);

    for (int i = 0; i < spec->n_nodes; i++)
        raw("%d ", route_pop[0].hops[i]);
    raw("\n");
#else
    int vehicles;
    calc_cvrp_cost(curr_cvrp, &best_cost, &vehicles, true);
    ERASE_LINE(); raw("Best route has cost %.2f using %d trucks\n",
                      best_cost, vehicles);
#endif
}



bool is_ga_initialised()
{
    return initialised;
}

void init_cvrp()
{
    msg("*** CVRP MODE ***\n");
    msg("Creating initial state...\n");

    //initialise adjacent "map"
    curr_cvrp = calloc(1, sizeof(*curr_cvrp));
    curr_cvrp->adjm = calloc(spec->n_nodes, sizeof(*(curr_cvrp->adjm)));
    for (int i = 0; i < spec->n_nodes; i++)
        curr_cvrp->adjm[i] = calloc(2, sizeof(**(curr_cvrp->adjm))); //each node must always connect to 2 other nodes

    //initially, node 0 connects to all customer nodes
    //all customer nodes also connect back to node 0
    for (int i = 1; i < spec->n_nodes; i++)
    {
        use_arc(curr_cvrp, 0, i);
        use_arc(curr_cvrp, i, 0);
    }

#if TEST_COST == 1
    msg("Testing cost computation...\n");

    const float cvrp_test_correct_cost = 41368.61f;
    float cvrp_test_cost;
    int cvrp_test_vehicles;

    calc_cvrp_cost(curr_cvrp, &cvrp_test_cost, &cvrp_test_vehicles, false);
    float cvrp_test_diff = cvrp_test_cost - cvrp_test_correct_cost;
    msg("calc_cvrp_cost() test: %.2f (actual=%.2f, diff=%.2f)", cvrp_test_cost, cvrp_test_correct_cost, cvrp_test_diff);
    if (abs(cvrp_test_diff) > 0.01 ||
            isnan(cvrp_test_cost) ||
            isinf(cvrp_test_cost))
    {
        raw("\n");
        die("CVRP cost test failed - computation of costs is wrong!\n");
    }
    else if (get_log_level() <= LOG_MSG);
    {
        raw(" - OK \n");
    }

    cvrp_test_diff = curr_cvrp->cost - cvrp_test_correct_cost;
    msg("cvrp.cost test: %.2f (actual=%.2f, diff=%.2f)", cvrp_test_cost, curr_cvrp->cost, cvrp_test_diff);
    if (abs(cvrp_test_diff) > 0.01 ||
            isnan(cvrp_test_cost) ||
            isinf(cvrp_test_cost))
    {
        raw("\n");
        die("CVRP cost test failed - computation of costs is wrong!\n");
    }
    else if (get_log_level() <= LOG_MSG);
    {
        raw(" - OK \n");
    }
#endif

    msg("Worst possible cost: %.2f\n", curr_cvrp->cost);

    //compute savings
    savings = calloc(spec->n_savings, sizeof(*(savings)));
    for (int i = 1, sid = 0; i < spec->n_nodes; i++)
    {
        for (int j = 1; j < spec->n_nodes; j++)
        {
            if (i != j)
            {
                //if node i -> node j, then node j -x- node 0, node i -x- node 0
                savings[sid].src = i;
                savings[sid].dst = j;
                savings[sid].saving = i == j ? 0 : (distm[i][0] + distm[0][j] - distm[i][j]);

                sid++;
            }
        }
    }

    sort_savings(0, spec->n_savings - 1);

    for (int i = 0; i < spec->n_savings; i++)
    {
        cw_saving s = savings[i];
        prf("Saving: %d -> %d = %.4f\n", s.src, s.dst, s.saving);
    }
}

//simple in-place quick sort descending
void sort_savings(const int l, const int r)
{
    if (r - l < 1)
        return;

    //pick pivot
    float pivot = savings[l].saving;
    int nl = l, nr = r;

    while (nl <= nr) {

        //find inconsistent pair
        while (nl < r && savings[nl].saving > pivot)
            nl++;

        while (nr > l && savings[nr].saving < pivot)
            nr--;

        if (nl <= nr) {
            if (savings[nl].saving != savings[nr].saving)
            {
                //swap
                cw_saving tmp = savings[nl];
                savings[nl] = savings[nr];
                savings[nr] = tmp;
            }

            nl++;
            nr--;
        }
    }

    //sort partitions
    sort_savings(l, nr);
    sort_savings(nl, r);
}

void init_tsp()
{
    msg("*** TSP MODE ***\n");

#if TEST_COST == 1
    msg("Testing cost computation...\n");
    route r = {calloc(spec->n_nodes, sizeof(*(route_pop[0].hops))), NULL, INFINITY};
    for (int i = 0; i < spec->n_nodes; i++)
        r.hops[i] = tsp_test_idx[i] + 1;

    float tsp_test_cost = calc_route_cost(&r), tsp_test_diff = tsp_test_cost - tsp_test_correct_cost;
    msg("calc_route_cost() test: %.2f (actual=%.2f, diff=%.2f)", tsp_test_cost, tsp_test_correct_cost, tsp_test_diff);
    if (abs(tsp_test_diff) > 0.01 ||
            isnan(tsp_test_cost) ||
            isinf(tsp_test_cost))
    {
        die("\nTSP cost test failed - computation of costs is wrong!\n");
    }
    else if (get_log_level() <= LOG_MSG);
    {
        raw(" - OK \n");
    }
#endif

    msg("Creating initial population...\n");

    //initialise ancestor generation
    route_pop = calloc(pop_size, sizeof(*route_pop));
    for (int i = 0; i < pop_size; i++)
        for (int j = 0; j < spec->n_nodes; j++)
        {
            route_pop[i].hops = calloc(spec->n_nodes, sizeof(*(route_pop[i].hops)));
            route_pop[i].edge_lengths = calloc(spec->n_nodes, sizeof(*(route_pop[i].edge_lengths)));
        }

    #pragma omp parallel
    {
        unsigned t_seed = rand_seed + omp_get_thread_num();

        #pragma omp for simd
        for (int i = 0; i < pop_size; i++)
            gen_random_route(&t_seed, &(route_pop[i]));
    }

    if (get_log_level() <= LOG_PRF)
    {
        const int pop_print_cap = ITERATION_PRINT_SAMPLES;
        prf("Initial population (sample of %d): \n", pop_print_cap);
        for (int i = 0; i < pop_size && i < pop_print_cap; i++)
        {
            prf("route_pop[%d]: ", i);
            for (int j = 0; j < spec->n_nodes; j++)
                raw("%d ", route_pop[i].hops[j]);
            raw("\n");
        }
    }
}

void init_ga(specification *_spec)
{
    spec = _spec;

    msg("RNG seed set to: 0x%x\n", rand_seed);
    msg("Population size set to: %d\n", pop_size);

    //initialise distance matrix
    distm = calloc(spec->n_nodes, sizeof(*(distm)));
    for (int i = 0; i < spec->n_nodes; i++)
        distm[i] = calloc(spec->n_nodes, sizeof(**(distm)));

    //compute distance matrix - this will be constant
    for (int i = 0; i < spec->n_nodes; i++)
        for (int j = 0; j < spec->n_nodes; j++)
            distm[i][j] = euclidean(&(spec->nodes[i]), &(spec->nodes[j]));

#if TSP_MODE == 1
    init_tsp();
#else
    init_cvrp();
#endif

    initialised = true;
}



void set_rand_seed(unsigned value)
{
    rand_seed = value;
}

void set_pop_size(long value)
{
    pop_size = value;
    if (pop_size < 0)
        die("Population size cannot be a negative number\n");
}

void set_survival_rate(float value)
{
    survival_rate = value;
    if (survival_rate < 0.0f || survival_rate > 1.0f)
        die("Invalid survival rate - must satisfy 0.0f < rate <= 1.0f\n");
}

void set_newcomer_rate(float value)
{
    newcomer_rate = value;
    if (newcomer_rate < 0.0f || newcomer_rate > 1.0f)
        die("Invalid newcomer rate - must satisfy 0.0f < rate <= 1.0f\n");
}

void set_mutation_rate(float value)
{
    mutation_rate = value;
    if (mutation_rate < 0.0f || mutation_rate > 1.0f)
        die("Invalid mutation rate - must satisfy 0.0f < rate <= 1.0f\n");
}

void set_converge_factor(int value)
{
    converge_factor = value;
    if (converge_factor < 0)
        die("Converge factor cannot be negative\n");
}

void set_apply_rate_ub(float value)
{
    apply_rate_ub = value;
    if (apply_rate_ub < 0.0f || apply_rate_ub > 1.0f)
        die("Invalid apply rate upper bound - must satisfy 0.0f < rate <= 1.0f\n");
}

void set_apply_rate_lb(float value)
{
    apply_rate_lb = value;
    if (apply_rate_lb < 0.0f || apply_rate_lb > 1.0f)
        die("Invalid apply rate lower bound - must satisfy 0.0f < rate <= 1.0f\n");
}