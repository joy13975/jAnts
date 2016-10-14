#include <stdlib.h>
#include <math.h>

#include "util.h"
#include "input_parser.h"
#include "omp.h"

#define LOG_LEVEL                   LOG_DBG//LOG_MSG
#define USE_TINYTEST                1

#define ACCESS_CHROMO(pop, i)       pop[n_nodes * (i)]
#define ACCESS_GENE(pop, i, j)      pop[n_nodes * (i) + (j)]

//GA
#define DEFAULT_RAND_SEED           0xdeadbeef
#define DEFAULT_POP_SIZE            100
#define DEFAULT_SURVIVE_RATE        0.1f //10% of population survive
#define DEFAULT_NEWCOMER_RATE       0.2f //20% of new generation are newcomers

#if USE_TINYTEST == 1
#define INPUT_FILE                  "tinytest.txt"
#else
#define INPUT_FILE                  "fruitybun250.vrp"
#endif

unsigned rand_seed                  = DEFAULT_RAND_SEED;
long pop_size                       = DEFAULT_POP_SIZE;
float survive_rate                  = DEFAULT_SURVIVE_RATE;
float newcomer_rate                 = DEFAULT_NEWCOMER_RATE;

int n_nodes;                        //number of nodes parsed from input
float *Xs, *Ys;                     //X and Y coordinate vectors
int *Ds;                            //demand vector (1D)
int *Cs;                            //chromosome vector (2D)
float *Fs;                          //fitness vector (1D)

const argument_format af_help       = {"-h", "--help", "Print the help message"};
const argument_format af_seed       = {"-r", "--seed", "Set the starting RNG seed"};
const argument_format af_pop        = {"-p", "--popation", "Set the population size"};
const argument_format af_svr        = {"-s", "--survive_rate", "Set the ratio of survivors"};
const argument_format af_ncr        = {"-n", "--newcomer_rate", "Set the ratio of newcomers"};

inline float compute_squared_cost(const int idx)
{
#ifdef USE_TINYTEST
    int *chromo = &ACCESS_CHROMO(Cs, idx);
    float cost = 0.0f;
    for (int i = 1; i < n_nodes; i++)
        cost += abs(Xs[chromo[i]] - Xs[chromo[i - 1]]) + abs(Ys[chromo[i]] - Ys[chromo[i - 1]]);

    cost += abs(Xs[chromo[n_nodes - 1]] - Xs[chromo[n_nodes - 1 - 1]]) +
            abs(Ys[chromo[n_nodes - 1]] - Ys[chromo[n_nodes - 1 - 1]]);
#else
    die("Unimplemented\n");
#endif

    return cost;
}

void run_ga()
{
    //initialise fitness vector
    Fs = calloc(pop_size, sizeof(*Fs));

    //convert rates into integer numbers
    const long n_survivors = pop_size * survive_rate;
    const long n_newcomers = pop_size * newcomer_rate;

    long generation = 1;

    #pragma omp parallel
    {
        unsigned tseed = rand_seed + omp_get_thread_num();

        #pragma omp single
        {
            msg("Running GA with %d threads\n", omp_get_num_threads());
        }

        while (generation++)
        {
            //compute SQUARED COST (final result needs to be rooted)
            #pragma omp for simd
            for (int i = 0; i < pop_size; i++)
                Fs[i] = compute_squared_cost(i);

            //pick out good parents (low squared cost)

            #pragma omp single
            {
                raw("Generation #%d\n", generation);

                raw("Best cost: %.2f\n", -13371337.0f);
            }

            //generate children

            //generate newcomers

            #pragma omp barrier
        }
    }
}

void init_ga()
{
    //initialise ancestor generation
    Cs = calloc(pop_size, sizeof(*Cs) * n_nodes);
    dbg("Cs is at %p, %ld bytes\n", Cs, pop_size * sizeof(*Cs) * n_nodes);

    #pragma omp parallel
    {
        unsigned tseed = rand_seed + omp_get_thread_num();

        #pragma omp for simd
        for (int i = 0; i < pop_size; i++)
        {
            for (int node_id = 1; node_id <= n_nodes; node_id++)
            {
                //find suitable index to fill next node id
                int fill_idx;
                do
                {
                    fill_idx = rand_r(&tseed) % n_nodes;
                }
                while (ACCESS_GENE(Cs, i, fill_idx));

                ACCESS_GENE(Cs, i, fill_idx) = node_id;
            }
        }
    }

    const int pop_show_cap = 10;
    dbg("Initial population (sample of %d): \n", pop_show_cap);
    for (int i = 0; i < pop_size && i < pop_show_cap; i++)
    {
        dbg("Cs[%d]: ", i);
        for (int j = 0; j < n_nodes; j++)
            raw("%d ", ACCESS_GENE(Cs, i, j));
        raw("\n");
    }
}

void print_help_and_exit()
{
    const int af_indents = 3;
    raw("jsolve - Joy's CVRP solver\n");
    raw("Usage: ./jtruck [COMMANDS] [OPTIONS]\n");
    raw("       Commands:\n");
    print_help_arguement(af_indents, af_help);
    raw("       Options:\n");
    print_help_arguement(af_indents, af_seed);
    print_help_arguement(af_indents, af_pop);
    print_help_arguement(af_indents, af_svr);
    print_help_arguement(af_indents, af_ncr);

    exit(1);
}

void parse_args(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++)
    {
        char *arg = argv[i];
        if (check_arg(arg, af_help))
        {
            print_help_and_exit();
        }

        //arguments below are in pairs
        if (argc == i + 1)
        {
            err("Argument: \"%s\" requires a value!\n", arg);
            print_help_and_exit();
        }

        if (check_arg(arg, af_seed))
        {
            rand_seed = parse_long(argv[++i]);
        }
        else if (check_arg(arg, af_pop))
        {
            pop_size = parse_long(argv[++i]);
            if (pop_size < 0)
                die("Population size cannot be a negative number\n");
        }
        else if (check_arg(arg, af_svr))
        {
            survive_rate = parse_float(argv[++i]);
            if (survive_rate < 0.0f || survive_rate >= 1.0f)
                die("Invalid survival rate - must satisfy 0.0f < rate < 1.0f\n");
        }
        else if (check_arg(arg, af_ncr))
        {
            newcomer_rate = parse_float(argv[++i]);
            if (newcomer_rate < 0.0f || newcomer_rate >= 1.0f)
                die("Invalid newcomer rate - must satisfy 0.0f < rate < 1.0f\n");
        }
        else
        {
            err("Unknown argument: \"%s\"\n", arg);
            print_help_and_exit();
        }
    }

    msg("Starting RNG seed set to: 0x%x\n", rand_seed);
    msg("Population size set to: %d\n", pop_size);
}

void on_sigint(int sig)
{
    raw("\n");
    wrn("Solver interrupted!\n");
    wrn("Attempting to output latest results...\n");
    die("Unimplemented\n");
}

int main(int argc, char *argv[])
{
    set_log_level(LOG_LEVEL);

    if (signal(SIGINT, on_sigint) == SIG_ERR)
        die("Couldn't hook signal handler\n");

    msg("Solver starting\n");

    //parse optional arguments
    parse_args(argc, argv);

    //seed RNG
    srand(rand_seed);

    //parse input file into arrays
    parse_input(INPUT_FILE, USE_TINYTEST, &n_nodes, &Xs, &Ys, &Ds);

    //go GA setups
    init_ga();

    //run my genetic algorithm
    run_ga();

    msg("Solver exiting\n");
    return 0;
}