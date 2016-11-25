#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <iomanip>

#include "config.h"
#include "typedefs.h"
#include "util.h"
#include "spec.h"
#include "input_parser.h"
#include "basic_random.h"
#include "route.h"
#include "output_writer.h"
#include "ants.h"
#include "basic_exchange.h"

const argument_format af_help       = {"-h", "--help", 0, "Print help message"};
const argument_format af_brand      = {"-br", "--basicrand", 0, "Do basic random search"};
const argument_format af_exc        = {"-ex", "--exchange", 0, "Do basic exchange search"};
const argument_format af_grid       = {"-gr", "--grid", 0, "Do grid search on ACO"};

const argument_format af_loglv      = {"-lg", "--loglv", 1, "Set log level {0-6}"};
const argument_format af_input      = {"-i", "--input", 1, "Set input file"};
const argument_format af_output     = {"-o", "--output", 1, "Set output file"};
const argument_format af_seed       = {"-r", "--seed", 1, "Set starting RNG seed"};
const argument_format af_pop        = {"-p", "--population", 1, "Set population size"};
const argument_format af_stg        = {"-mxs", "--maxstagnancy", 1, "Set stopping stagnant iterations"};
const argument_format af_alpha      = {"-a", "--alpha", 1, "Set importance of distance in ACO"};
const argument_format af_beta       = {"-b", "--beta", 1, "Set importance of pheromone in ACO"};
const argument_format af_pers       = {"-ps", "--persistence", 1, "Set persistence of pheromone in ACO"};
const argument_format af_nbh        = {"-nd", "--nbhooddiv", 1, "Set nbhood divisor in ACO"};
const argument_format af_mnph       = {"-mnp", "--minphero", 1, "Set min pheromone in ACO"};


#define FOREACH_SEARCH_MODE(MACRO) \
    MACRO(MODE_BRAND) \
    MACRO(MODE_EXCHANGE) \
    MACRO(MODE_ACO)

DECL_ENUM_AND_STRING(Search_Mode, FOREACH_SEARCH_MODE);

int rand_seed                   = DEFAULT_RAND_SEED;
double start_time               = -1;
String input_file               = DEFAULT_INPUT_FILE;
String output_file              = DEFAULT_OUTPUT_FILE;
String data_output_file         = DEFAULT_DATA_OUTPUT_FILE;
String grid_output_file         = DEFAULT_GRID_OUTPUT_FILE;
Search_Mode search_mode         = MODE_ACO;
long population_size            = DEFAULT_POPULATION_SIZE;
long max_stagnancy              = DEFAULT_MAX_STAGNANCY;
float aco_alpha                 = DEFAULT_ACO_ALPHA;
float aco_beta                  = DEFAULT_ACO_BETA;
float aco_pers                  = DEFAULT_ACO_PERSISTENCE;
float aco_min_phero             = DEFAULT_ACO_MIN_PHERO;
int aco_nbhood_div              = DEFAULT_ACO_NBHOOD_DIV;
int failure_count               = 0;
bool do_grid_search             = false;
Route best_route                = Route::Dummy();
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
    print_help_arguement(af_exc);
    print_help_arguement(af_grid);
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
        else if (next_arg_matches(af_exc))
        {
            search_mode = MODE_EXCHANGE;
        }
        else if (next_arg_matches(af_grid))
        {
            do_grid_search = true;
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
            aco_nbhood_div = parse_long(next_arg());
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

        writeStrStream(data_output_file, data_stream);
        writeSolution(output_file, best_route);

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
            signal(SIGSEGV, on_failure) == SIG_ERR ||
            signal(SIGILL, on_failure) == SIG_ERR)
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
    case MODE_EXCHANGE:
    {
        msg("Running basic exchange search\n");
        BasicExchange::search(best_route, spec, data_stream, start_time);
        break;
    }
    case MODE_ACO:
    {
        if (do_grid_search)
        {
            const float gridAlphas[]    = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
            const float gridBetas[]     = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
            const float gridPerss[]     = {0.90f, 0.92f, 0.94f, 0.96f, 0.98f, 0.99f};
            const float gridMinPheros[] = {0.00f, 0.01f, 0.02f, 0.03f, 0.04f, 0.05f};
            const int gridNBHoodDivs[]  = {3, 4, 5, 10, 15, 20, 30};

            for (const float& alpha : gridAlphas)
            {
                for (const float& beta : gridBetas)
                {
                    for (const float& pers : gridPerss)
                    {
                        for (const float& minPhero : gridMinPheros)
                        {
                            for (const int& nbhoodDiv : gridNBHoodDivs)
                            {
                                start_time = get_timestamp_us();
                                msg("Running ACO search\n");
                                Ants(spec,
                                     population_size,
                                     max_stagnancy,
                                     alpha,
                                     beta,
                                     aco_pers,
                                     minPhero,
                                     nbhoodDiv,
                                     data_stream).search(best_route, start_time);

                                const float bestCost = best_route.calcScoreSerious();
                                const double antTime = (get_timestamp_us() - start_time) / 1e6;
                                msg("Best cost: %.2f in %.2fs\n", bestCost, antTime);
                                std::stringstream grid_stream;
                                grid_stream << std::fixed << std::setprecision(1) << alpha << ", "
                                            << std::fixed << std::setprecision(1) << beta << ", "
                                            << std::fixed << std::setprecision(2) << pers << ", "
                                            << std::fixed << std::setprecision(2) << minPhero << ", "
                                            << nbhoodDiv << ", "
                                            << std::fixed << std::setprecision(16) << bestCost << ", "
                                            << std::fixed << std::setprecision(2) << antTime
                                            << "\n";

                                writeStrStreamMode(grid_output_file, grid_stream, std::ios_base::app);
                                writeStrStream(data_output_file, data_stream);
                                writeSolution(output_file, best_route);
                            }
                        }
                    }
                }
            }
        }
        else
        {
            msg("Running ACO search\n");
            Ants(spec,
                 population_size,
                 max_stagnancy,
                 aco_alpha,
                 aco_beta,
                 aco_pers,
                 aco_min_phero,
                 aco_nbhood_div,
                 data_stream).search(best_route, start_time);
        }
        break;
    }
    default:
        die("Unknown search mode: %s", Search_Mode_String[search_mode]);
    }

    finalise_and_exit(0);
}
