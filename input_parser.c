#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "input_parser.h"
#include "util.h"

#define BUFFER_SIZE 256

void parse_input(const char* input_file,
                 bool jformat,
                 int *n_nodes_ptr,
                 float **Xs_ptr,
                 float **Ys_ptr,
                 int **Ds_ptr)
{
    msg("Parsing input: %s\n", input_file)
    FILE *fp;
    if (!(fp = fopen(input_file, "r")))
        die("Can't open input file \"%s\" (%s)\n", input_file, get_error_string());

    float *Xs, *Ys;
    int *Ds;
    long n_nodes;

    char line_buffer[BUFFER_SIZE] = {0};
    for (int lineno = 0; fgets(line_buffer, BUFFER_SIZE, fp); lineno++)
    {
        if (jformat)
        {
            if (!lineno)
            {
                //first line is the number of nodes
                line_buffer[strlen(line_buffer) - 1] = '\0';
                n_nodes = parse_long(line_buffer);
                Xs = calloc(n_nodes, sizeof(**Xs_ptr));
                Ys = calloc(n_nodes, sizeof(**Ys_ptr));
                //tiny TSP test has no demand vector Ds
            }
            else
            {
                if (lineno - 1 >= n_nodes)
                {
                    wrn("Extra lines in input file not parsed (n_nodes=%d)\n", n_nodes);
                    break;
                }
                sscanf(line_buffer, "%f %f", &(Xs[lineno - 1]), &(Ys[lineno - 1]));
            }
        }
        else
        {
            die("Unimplemented\n");
        }
    }

    *Xs_ptr     = Xs;
    *Ys_ptr     = Ys;
    *Ds_ptr     = Ds;
    *n_nodes_ptr = n_nodes;

    msg("Number of nodes: %d\n", n_nodes);

    const int node_print_cap = 10;
    dbg("Coordinates (sample of %d):\n", node_print_cap);
    for (int i = 0; i < n_nodes && i < node_print_cap; i++)
        dbg("%.0f, %.0f\n", Xs[i], Ys[i]);
}