#include <iostream>
#include <fstream>
#include <iomanip>

#include "output_writer.h"
#include "util.h"

using namespace std;

void write_output(const String output_file, const Route& bestRoute)
{
    msg("Writing best solution to file \"%s\"\n", output_file.c_str());
    ofstream os(output_file);
    if (!os.is_open())
        die("Can't open output file \"%s\" (%s)\n", output_file.c_str(), get_error_string());

    //write file header
    os << "login cy13308 67678\n";
    os << "name Joy Yeh\n";
    os << "algorithm Tabu search with Clark & Wright's savings heuristic\n";
    os << "cost " << std::setprecision(16) << bestRoute.getScore() << "\n";
    os << bestRoute.genStr();

    os.close();
}