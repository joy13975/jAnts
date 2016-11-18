#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "input_parser.h"
#include "util.h"

#define BUFFER_SIZE 256
typedef enum { UNSET, COORD_MODE, DEMAND_MODE } read_mode;

void parse_input(const String input_file,
                 Spec& spec)
{
    msg("Parsing input from file \"%s\"\n", input_file.c_str());
    FILE *fp;
    if (!(fp = fopen(input_file.c_str(), "r")))
        die("Can't open input file \"%s\" (%s)\n", input_file.c_str(), get_error_string());

    read_mode mode = UNSET;
    int mode_start_line = -1;
    Nodes nodes;

    char line_buffer[BUFFER_SIZE] = {0};
    for (int lineno = 0; fgets(line_buffer, BUFFER_SIZE, fp); lineno++)
    {
        if (lineno == 0)
        {
            //first line is the number of nodes
            int dim;
            sscanf(line_buffer, "DIMENSION : %d", &dim);
            spec.setDim(dim);

            for (int i = 0; i < dim; i++)
                nodes.push_back(Node(0, 0, 0));
        }
        else if (lineno == 1)
        {
            int capacity;
            sscanf(line_buffer, "CAPACITY : %d", &(capacity));
            spec.setVCap(capacity);
        }
        else
        {
            if (mode == UNSET)
            {
                if (!strcmp(line_buffer, "NODE_COORD_SECTION\n"))
                    mode = COORD_MODE;
                else if (!strcmp(line_buffer, "DEMAND_SECTION\n"))
                    mode = DEMAND_MODE;
                else
                    die("Unknown section string: \"%s\"\n", line_buffer);

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
                    if (idx < 1 || idx > spec.getDim())
                        die("Node index %d is out of range (1-%d)\n", idx, spec.getDim());
                    nodes[idx - 1].x = x;
                    nodes[idx - 1].y = y;
                }
                else if (mode == DEMAND_MODE)
                {
                    int demand;
                    sscanf(line_buffer, "%d %d", &idx, &demand);
                    if (idx < 1 || idx > spec.getDim())
                        die("Node index %d is out of range (1-%d)\n", idx, spec.getDim());
                    nodes[idx - 1].z = demand;
                }

                if (lineno - mode_start_line >= spec.getDim())
                    mode = UNSET;
            }
        }
    }

    spec.setNodes(nodes);

    msg("Number of nodes: %d\n", spec.getDim());

    for (int i = 0; i < spec.getDim(); i++)
    {

        Node n = nodes[i];
        prf("Node #%d: (%d, %d) -> %d\n", i + 1, n.x, n.y, n.z);
    }
}