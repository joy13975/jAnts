#ifndef _INPUT_PARSER_H
#define _INPUT_PARSER_H

#include <stdbool.h>

void parse_input(const char* input_file,
                 bool jformat,
                 int *nnodes_ptr,
                 float **Xs_ptr,
                 float **Ys_ptr,
                 int **Ds_ptr);

#endif
