#ifndef _INPUT_PARSER_H
#define _INPUT_PARSER_H

#include <stdbool.h>
#include "typedefs.h"
#include "spec.h"

void parse_input(const String input_file,
                 Spec& spec);

#endif /* include guard */