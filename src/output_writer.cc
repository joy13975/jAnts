#include <iostream>
#include <fstream>
#include <iomanip>

#include "output_writer.h"
#include "util.h"

void solutionToStrStream(const String output_file,
                         const Route& bestRoute,
                         std::stringstream& ss)
{
    ss << "login cy13308 67678\n";
    ss << "name Joy Yeh\n";
    ss << "algorithm Ant colony optimisation with Clark & Wright's savings heuristic\n";
    ss << "cost " << std::fixed  << std::setprecision(16) << bestRoute.calcScoreSerious() << "\n";
    ss << Route::genStr(bestRoute.getHops());
}


void writeStrStream(const String output_file, const std::stringstream& ss)
{
    writeStrStreamMode(output_file, ss);
}

void writeStrStreamMode(const String output_file,
                        const std::stringstream& ss,
                        std::ios_base::openmode mode)
{
    std::ofstream os(output_file, mode);
    if (!os.is_open())
        die("Can't open file \"%s\" (%s)\n", output_file.c_str(), get_error_string());
    os << ss.rdbuf();
    os.close();
}