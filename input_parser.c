#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "input_parser.h"
#include "util.h"

#define BUFFER_SIZE 256
typedef enum { UNSET, COORD_MODE, DEMAND_MODE } read_mode;

void parse_input(const char* input_file,
                 specification **spec_ptr)
{
    msg("Parsing input: %s\n", input_file)
    FILE *fp;
    if (!(fp = fopen(input_file, "r")))
        die("Can't open input file \"%s\" (%s)\n", input_file, get_error_string());

    specification *spec = calloc(1, sizeof(specification));
    read_mode mode = UNSET;
    int mode_start_line = -1;

    char line_buffer[BUFFER_SIZE] = {0};
    for (int lineno = 0; fgets(line_buffer, BUFFER_SIZE, fp); lineno++)
    {
        if (lineno == 0)
        {
            //first line is the number of nodes
            sscanf(line_buffer, "DIMENSION : %d", &(spec->n_nodes));
            spec->nodes = calloc(spec->n_nodes, sizeof(*(spec->nodes)));
            spec->n_savings = spec->n_nodes * spec->n_nodes;
        }
        else if (lineno == 1)
        {
            sscanf(line_buffer, "CAPACITY : %d", &(spec->capacity));
        }
        else
        {
            if (mode == UNSET)
            {
                if (!strcmp(line_buffer, "NODE_COORD_SECTION\n"))
                    mode = COORD_MODE;
                else if (!strcmp(line_buffer, "DEMAND_SECTION\n"))
                    mode = DEMAND_MODE;

                if (mode == UNSET)
                {
                    die("Error while parsing input:\n\tExpecting \"NODE_COORD_SECTION\" or \"DEMAND_SECTION\", but got \"%s\"\n", line_buffer);
                }
                else
                {
                    mode_start_line = lineno;
                }
            }
            else
            {
                int idx;
                if (mode == COORD_MODE)
                {
                    int x, y;
                    sscanf(line_buffer, "%d %d %d", &idx, &x, &y);
                    spec->nodes[idx-1].x = x;
                    spec->nodes[idx-1].y = y;
                }
                else if (mode == DEMAND_MODE)
                {
                    int demand;
                    sscanf(line_buffer, "%d %d", &idx, &demand);
                    spec->nodes[idx].demand = demand;
                }

                if (lineno - mode_start_line >= spec->n_nodes)
                    mode = UNSET;
            }
        }
    }

    *spec_ptr = spec;

    msg("Number of nodes: %d\n", spec->n_nodes);

    for (int i = 0; i < spec->n_nodes; i++)
    {
        node n = spec->nodes[i];
        prf("Node #%d: (%d, %d) -> %d\n", i + 1, n.x, n.y, n.demand);
    }
}