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

    const String dimPrefix = "DIMENSION : ";
    const String capPrefix = "CAPACITY : ";
    const String nodeSecTag = "NODE_COORD_SECTION";
    const String demSecTag = "DEMAND_SECTION";
    int dim = -1;
    int capacity = -1;

    char line_buffer[BUFFER_SIZE] = {0};
    for (int lineno = 0; fgets(line_buffer, BUFFER_SIZE, fp); lineno++)
    {
        String lineStr = String(line_buffer);
        if (lineStr.compare(0, dimPrefix.size(), dimPrefix) == 0)
        {
            //first line is the number of nodes
            sscanf(line_buffer, "DIMENSION : %d", &dim);
            spec.setDim(dim);

            for (int i = 0; i < dim; i++)
                nodes.push_back(Node(0, 0, 0));
        }
        else if (lineStr.compare(0, capPrefix.size(), capPrefix) == 0)
        {
            sscanf(line_buffer, "CAPACITY : %d", &(capacity));
            spec.setVCap(capacity);
        }
        else
        {
            if (mode == UNSET)
            {
                if (lineStr.compare(0, nodeSecTag.size(), nodeSecTag) == 0)
                    mode = COORD_MODE;
                else if (lineStr.compare(0, demSecTag.size(), demSecTag) == 0)
                    mode = DEMAND_MODE;
                else
                    wrn("Ignoring line: %s\n", lineStr.substr(0, lineStr.size() - 1).c_str());

                if (mode != UNSET)
                {
                    mode_start_line = lineno;
                }
            }
            else
            {
                int idx;
                if (mode == COORD_MODE)
                {
                    float x, y;
                    sscanf(line_buffer, "%d %f %f", &idx, &x, &y);
                    if (idx < 1 || idx > spec.getDim())
                        die("Node index %d is out of range (1-%d)\n", idx, spec.getDim());
                    nodes[idx - 1].x = x;
                    nodes[idx - 1].y = y;
                }
                else if (mode == DEMAND_MODE)
                {
                    float demand;
                    sscanf(line_buffer, "%d %f", &idx, &demand);
                    if (idx < 1 || idx > spec.getDim())
                        die("Node index %d is out of range (1-%d)\n", idx, spec.getDim());
                    nodes[idx - 1].z = demand;
                }

                if (lineno - mode_start_line >= spec.getDim())
                    mode = UNSET;
            }
        }
    }

    if (dim == -1)
        die("Could not parse dimension\n");
    if (capacity == -1)
        die("Could not parse capacity\n");

    spec.setNodes(nodes);

    msg("Number of nodes: %d\n", spec.getDim());

    for (int i = 0; i < spec.getDim(); i++)
    {

        Node n = nodes[i];
        prf("Node #%d: (%.2f, %.2f) with demand %.2f\n", i + 1, n.x, n.y, n.z);
    }
}