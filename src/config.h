#ifndef _CONFIG_H
#define _CONFIG_H

#ifndef ITERATION_PRINT_SAMPLES
#define ITERATION_PRINT_SAMPLES     10
#endif

#define DEFAULT_LOG_LEVEL           LOG_DEBUG // LOG_WARN
#define DEFAULT_RAND_SEED           0xdeadbeef
#define DEFAULT_INPUT_FILE          "res/fruitybun250_2016.vrp"
#define DEFAULT_OUTPUT_FILE         "last-solution.txt"
#define DEFAULT_DATA_OUTPUT_FILE    "data.txt"
#define DEFAULT_GRID_OUTPUT_FILE    "grid.txt"
#define DEFAULT_POPULATION_SIZE     128
#define DEFAULT_MAX_STAGNANCY       150
#define DEFAULT_TIME_LIMIT_SEC      (60 * 999999) //some large number

#endif /* include guard */
