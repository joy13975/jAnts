#ifndef _INPUT_PARSER_H
#define _INPUT_PARSER_H

#include <stdbool.h>
#include "typedefs.h"

void parse_input(const char* input_file,
                 CVRP_Spec **spec_ptr);

#endif /* include guard */