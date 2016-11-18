#include <stdlib.h>

#include "config.h"
#include "util.h"
#include "input_parser.h"
#include "ga.h"

#define LOG_LEVEL                   LOG_DBG//LOG_MSG

#define INPUT_FILE                  "fruitybun250.vrp"

const argument_format af_help       = {"-h", "--help", 0, "Print the help message"};
const argument_format af_loglv      = {"-l", "--loglv", 1, "Set the log level {0-6}"};
const argument_format af_seed       = {"-r", "--seed", 1, "Set the starting RNG seed"};
const argument_format af_pop        = {"-p", "--population", 1, "Set the population size"};
const argument_format af_svr        = {"-s", "--survival_rate", 1, "Set the ratio of survivors"};
const argument_format af_ncr        = {"-n", "--newcomer_rate", 1, "Set the ratio of newcomers"};

double start_time = -1;

void print_help_and_exit()
{
    set_leading_spaces(12);
    raw("jsolve - Joy's CVRP solver\n");
    raw("Usage: ./jtruck [COMMANDS] [OPTIONS]\n");
    raw("       Commands:\n");
    print_help_arguement(af_help);
    raw("       Options:\n");
    print_help_arguement(af_loglv);
    print_help_arguement(af_seed);
    print_help_arguement(af_pop);
    print_help_arguement(af_svr);
    print_help_arguement(af_ncr);
    set_leading_spaces(0);

    exit(1);
}

void parse_args(int argc, char *argv[])
{
    init_args(argc, argv);

    bool loglv_set = false;
    while (have_next_arg())
    {
        if (next_arg_matches(af_help))
        {
            print_help_and_exit();
        }
        else if (next_arg_matches(af_loglv))
        {
            set_log_level(parse_long(next_arg()));
            loglv_set = true;
        }
        else if (next_arg_matches(af_seed))
        {
            set_rand_seed(parse_long(next_arg()));
        }
        else if (next_arg_matches(af_pop))
        {
            set_pop_size(parse_long(next_arg()));
        }
        else if (next_arg_matches(af_svr))
        {
            set_survival_rate(parse_float(next_arg()));
        }
        else if (next_arg_matches(af_ncr))
        {
            set_newcomer_rate(parse_float(next_arg()));
        }
        else
        {
            err("Unknown argument: \"%s\"\n", next_arg());
            print_help_and_exit();
        }
    }

    if (!loglv_set)
    {
        set_log_level(LOG_LEVEL);
    }
}

#include <stdio.h>
void on_sigint(int sig)
{
    ERASE_LINE(); printf("\n");
    ERASE_LINE(); printf("\n");

    ERASE_LINE(); printf("[WARNING] Solver interrupted!\n");
    ERASE_LINE(); printf("[WARNING] Attempting to output latest results...\n");

    if (is_ga_initialised())
    {
        print_best_solution();
    }
    else
    {
        printf("[WARNING] GA was not even initialised!\n");
    }

    printf("[MSG] Solver exiting - total time: %.2f s\n", (get_timestamp_us() - start_time) / 1e6);
    exit(1);
}

int main(int argc, char *argv[])
{
    if (signal(SIGINT, on_sigint) == SIG_ERR)
        die("Couldn't hook signal handler\n");

    start_time = get_timestamp_us();

    //parse optional arguments
    parse_args(argc, argv);

    msg("Solver starting\n");

    //parse input file into arrays
    specification *spec;
    parse_input(INPUT_FILE, &spec);
    spec->start_time_us = start_time;

    //go GA setups
    init_ga(spec);

    //run my genetic algorithm
    run_ga(start_time);

    free(spec->nodes);
    free(spec);

    msg("Solver exiting - total time: %.2f s\n", (get_timestamp_us() - start_time) / 1e6);
    return 0;
}