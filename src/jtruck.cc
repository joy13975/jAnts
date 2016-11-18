#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "typedefs.h"
#include "util.h"
#include "spec.h"
#include "input_parser.h"
#include "basic_random_search.h"
#include "route.h"
#include "output_writer.h"

const argument_format af_help       = {"-h", "--help", 0, "Print help message"};
const argument_format af_brand      = {"-br", "--basicrand", 1, "Do basic random search on N routes"};
const argument_format af_loglv      = {"-l", "--loglv", 1, "Set log level {0-6}"};
const argument_format af_input      = {"-i", "--input", 1, "Set input file"};
const argument_format af_output     = {"-o", "--output", 1, "Set output file"};
const argument_format af_seed       = {"-r", "--seed", 1, "Set starting RNG seed"};
// const argument_format af_pop        = {"-p", "--population", 1, "Set population size"};
// const argument_format af_svr        = {"-s", "--survival_rate", 1, "Set ratio of survivors"};
// const argument_format af_ncr        = {"-n", "--newcomer_rate", 1, "Set ratio of newcomers"};


#define FOREACH_SEARCH_MODE(MACRO) \
    MACRO(MODE_BRAND) \
    MACRO(MODE_FULL)

DECL_ENUM_AND_STRING(Search_Mode, FOREACH_SEARCH_MODE);

double start_time = -1;
int rand_seed = 0xdeadbeef;
String input_file = DEFAULT_INPUT_FILE;
String output_file = DEFAULT_OUTPUT_FILE;
Search_Mode search_mode = MODE_FULL;
long search_size = 10000;

void print_help_and_exit()
{
    raw("----------------------\n");
    raw("JTruck - A CVRP solver\n");
    raw("----------------------\n");
    raw("Usage: ./jtruck [COMMANDS] [OPTIONS]\n");
    raw("       Commands:\n");
    set_leading_spaces(8);
    print_help_arguement(af_help);
    set_leading_spaces(0);
    raw("       Options:\n");
    set_leading_spaces(8);
    print_help_arguement(af_loglv);
    print_help_arguement(af_seed);
    // print_help_arguement(af_pop);
    // print_help_arguement(af_svr);
    // print_help_arguement(af_ncr);
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
        else if (next_arg_matches(af_brand))
        {
            search_mode = MODE_BRAND;
            search_size = parse_long(next_arg());
        }
        else if (next_arg_matches(af_loglv))
        {
            set_log_level((Log_Level) parse_long(next_arg()));
            loglv_set = true;
        }
        else if (next_arg_matches(af_input))
        {
            input_file = next_arg();
        }
        else if (next_arg_matches(af_output))
        {
            output_file = next_arg();
        }
        else if (next_arg_matches(af_seed))
        {
            rand_seed = parse_long(next_arg());
        }
        // else if (next_arg_matches(af_pop))
        // {
        //     set_pop_size(parse_long(next_arg()));
        // }
        // else if (next_arg_matches(af_svr))
        // {
        //     set_survival_rate(parse_float(next_arg()));
        // }
        // else if (next_arg_matches(af_ncr))
        // {
        //     set_newcomer_rate(parse_float(next_arg()));
        // }
        else
        {
            err("Unknown argument: \"%s\"\n", next_arg());
            print_help_and_exit();
        }
    }

    if (!loglv_set)
    {
        set_log_level(DEFAULT_LOG_LEVEL);
    }
}

#include <stdio.h>
void on_sigint(int sig)
{
    ERASE_LINE(); printf("\n");
    ERASE_LINE(); printf("\n");

    ERASE_LINE(); printf("[WARNING] Solver interrupted\n");
    ERASE_LINE(); printf("[WARNING] Attempting to output latest results...\n");

    // if (is_ga_initialised())
    // {
    //     print_best_solution();
    // }
    // else
    // {
    //     printf("[WARNING] GA was not even initialised!\n");
    // }

    printf("Time: %.2f s\n", (get_timestamp_us() - start_time) / 1e6);
    exit(1);
}

int main(int argc, char *argv[])
{
    // if (signal(SIGINT, on_sigint) == SIG_ERR)
    //     die("Couldn't hook signal handler\n");

    start_time = get_timestamp_us();

    //parse optional arguments
    parse_args(argc, argv);

    //parse input file
    Spec spec;
    parse_input(input_file, spec);

    Route bestRoute = Route(spec);

    switch (search_mode)
    {
    case MODE_BRAND:
        msg("Running basic random search\n");
        bestRoute = basicRandomSearch(spec, search_size);
        break;
    case MODE_FULL:
        msg("Running full search\n");
        break;
    default:
        die("Unknown search mode: %s", Search_Mode_String[search_mode]);
    }

    msg("Best cost: %.2f\n", bestRoute.getScoreLazy());

    write_output(output_file, bestRoute);

    msg("Time: %.2f s\n", (get_timestamp_us() - start_time) / 1e6);
    return 0;
}