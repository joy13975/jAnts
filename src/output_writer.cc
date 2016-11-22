#include <iostream>
#include <fstream>
#include <iomanip>

#include "output_writer.h"
#include "util.h"

using namespace std;

void write_solution(const String output_file, const Route& bestRoute)
{
    std::stringstream ss;
    //write file header
    ss << "login cy13308 67678\n";
    ss << "name Joy Yeh\n";
    ss << "algorithm Ant colony optimisation with Clark & Wright's savings heuristic\n";
    ss << "cost " << std::fixed  << std::setprecision(16) << bestRoute.calcScoreSerious() << "\n";
    ss << Route::genStr(bestRoute.getHops());

    write_ss(output_file, ss);
}


void write_ss(const String output_file, const std::stringstream& ss)
{
    ofstream os(output_file);
    if (!os.is_open())
        die("Can't open file \"%s\" (%s)\n", output_file.c_str(), get_error_string());
    os << ss.rdbuf();
    os.close();
}