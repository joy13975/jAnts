#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include "config.h"
#include "typedefs.h"
#include "util.h"
#include "spec.h"
#include "input_parser.h"
#include "basic_random.h"
#include "route.h"
#include "output_writer.h"
#include "ants.h"

const argument_format af_help       = {"-h", "--help", 0, "Print help message"};
const argument_format af_brand      = {"-br", "--basicrand", 0, "Do basic random search"};

const argument_format af_loglv      = {"-lg", "--loglv", 1, "Set log level {0-6}"};
const argument_format af_input      = {"-i", "--input", 1, "Set input file"};
const argument_format af_output     = {"-o", "--output", 1, "Set output file"};
const argument_format af_seed       = {"-r", "--seed", 1, "Set starting RNG seed"};
const argument_format af_pop        = {"-p", "--population", 1, "Set population size"};
const argument_format af_stg        = {"-mxs", "--maxstagnancy", 1, "Set stopping stagnant iterations"};
const argument_format af_alpha      = {"-a", "--alpha", 1, "Set importance of distance in ACO"};
const argument_format af_beta       = {"-b", "--beta", 1, "Set importance of pheromone in ACO"};
const argument_format af_pers       = {"-ps", "--persistence", 1, "Set persistence of pheromone in ACO"};
const argument_format af_lamda      = {"-lb", "--lamda", 1, "Set number of exchanges in ACO"};
const argument_format af_nbh        = {"-n", "--neighbourhood", 1, "Set elite scope in ACO"};
const argument_format af_mnph       = {"-mnp", "--minphero", 1, "Set min pheromone in ACO"};


#define FOREACH_SEARCH_MODE(MACRO) \
    MACRO(MODE_BRAND) \
    MACRO(MODE_FULL)

DECL_ENUM_AND_STRING(Search_Mode, FOREACH_SEARCH_MODE);

int rand_seed                   = DEFAULT_RAND_SEED;
double start_time               = -1;
String input_file               = DEFAULT_INPUT_FILE;
String output_file              = DEFAULT_OUTPUT_FILE;
String data_output_file         = DEFAULT_DATA_OUTPUT_FILE;
Search_Mode search_mode         = MODE_FULL;
long population_size            = DEFAULT_POPULATION_SIZE;
long max_stagnancy              = DEFAULT_MAX_STAGNANCY;
float aco_alpha                 = DEFAULT_ACO_ALPHA;
float aco_beta                  = DEFAULT_ACO_BETA;
float aco_pers                  = DEFAULT_ACO_PERSISTENCE;
float aco_min_phero             = DEFAULT_ACO_MIN_PHERO;
int aco_nbhood                  = DEFAULT_ACO_NBHOOD;
int aco_lamda                   = DEFAULT_ACO_LAMDA;
Route best_route                = Route::Dummy();
int failure_count               = 0;
std::stringstream data_stream;

void print_help_and_exit()
{
    raw("----------------------\n");
    raw("JTruck - A CVRP solver\n");
    raw("----------------------\n");
    raw("Usage: ./jtruck [COMMANDS] [OPTIONS]\n");
    raw("       Commands:\n");
    set_leading_spaces(8);
    print_help_arguement(af_help);
    print_help_arguement(af_brand);
    set_leading_spaces(0);
    raw("       Options:\n");
    set_leading_spaces(8);
    print_help_arguement(af_loglv);
    print_help_arguement(af_input);
    print_help_arguement(af_output);
    print_help_arguement(af_seed);
    print_help_arguement(af_pop);
    print_help_arguement(af_stg);
    print_help_arguement(af_alpha);
    print_help_arguement(af_beta);
    print_help_arguement(af_pers);
    print_help_arguement(af_lamda);
    print_help_arguement(af_nbh);
    print_help_arguement(af_mnph);
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
        else if (next_arg_matches(af_pop))
        {
            population_size = parse_long(next_arg());
        }
        else if (next_arg_matches(af_stg))
        {
            max_stagnancy = parse_long(next_arg());
        }
        else if (next_arg_matches(af_alpha))
        {
            aco_alpha = parse_float(next_arg());
        }
        else if (next_arg_matches(af_beta))
        {
            aco_beta = parse_float(next_arg());
        }
        else if (next_arg_matches(af_pers))
        {
            aco_pers = parse_float(next_arg());
        }
        else if (next_arg_matches(af_mnph))
        {
            aco_min_phero = parse_float(next_arg());
        }
        else if (next_arg_matches(af_nbh))
        {
            aco_nbhood = parse_long(next_arg());
        }
        else if (next_arg_matches(af_lamda))
        {
            aco_lamda = parse_long(next_arg());
        }
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

void finalise_and_exit(int default_sig)
{
    msg("Time: %.2f s\n", (get_timestamp_us() - start_time) / 1e6);

    if (best_route.isDummy())
    {
        msg("No route was computed!\n");
        unlink(data_output_file.c_str());
        unlink(output_file.c_str());

        exit(1);
    }
    else
    {
        msg("Best cost: %.2f\n", best_route.calcScoreSerious());

        write_ss(data_output_file, data_stream);
        write_solution(output_file, best_route);

        exit(default_sig);
    }
}

void on_failure(int sig)
{
    failure_count++;

    // if the follow block fails again, just exit
    if (failure_count == 1)
    {
        ERASE_LINE(); printf("\n");

        ERASE_LINE(); printf("[WARNING] Failure trap (%s)\n", strsignal(sig));
        ERASE_LINE(); printf("[WARNING] Attempting to output latest results...\n");

        finalise_and_exit(sig);
    }

    exit(sig);
}

int main(int argc, char *argv[])
{
    if (signal(SIGINT, on_failure) == SIG_ERR ||
            signal(SIGSEGV, on_failure) == SIG_ERR)
        die("Couldn't hook signal handler\n");

    start_time = get_timestamp_us();

    //parse optional arguments
    parse_args(argc, argv);

    //parse input file
    Spec spec(rand_seed);
    parse_input(input_file, spec);

    switch (search_mode)
    {
    case MODE_BRAND:
    {
        msg("Running basic random search\n");
        BasicRandom::search(best_route, spec, population_size, data_stream);
        break;
    }
    case MODE_FULL:
    {
        msg("Running full search\n");
        Ants(spec,
             population_size,
             max_stagnancy,
             aco_alpha,
             aco_beta,
             aco_pers,
             aco_min_phero,
             aco_nbhood,
             aco_lamda,
             data_stream).search(best_route, start_time);
        break;
    }
    default:
        die("Unknown search mode: %s", Search_Mode_String[search_mode]);
    }

    finalise_and_exit(0);
}
