#ifndef _GA_H
#define _GA_H

#include <stdbool.h>

#include "jsolve_types.h"

void print_best_solution();
void run_ga();
bool is_ga_initialised();
void init_ga(specification *_spec);
void set_rand_seed(unsigned value);
void set_pop_size(long value);
void set_survival_rate(float value);
void set_newcomer_rate(float value);
void set_crossover_rate(float value);
void set_mutation_rate(float value);
void set_converge_factor(int value);
void set_apply_rate_ub(float value);
void set_apply_rate_lb(float value);

#endif /* include guard */