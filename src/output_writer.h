#ifndef _OUTPUT_WRITER_H_
#define _OUTPUT_WRITER_H_

#include <sstream>

#include "typedefs.h"
#include "route.h"

void write_solution(const String output_file, const Route& bestRoute);

void write_ss(const String output_file, const std::stringstream& ss);

#endif /* include guard */