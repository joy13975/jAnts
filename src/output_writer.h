#ifndef _OUTPUT_WRITER_H_
#define _OUTPUT_WRITER_H_

#include <sstream>

#include "typedefs.h"
#include "route.h"

void solutionToStrStream(const String output_file,
                         const Route& bestRoute,
                         std::stringstream& ss);

void writeStrStream(const String output_file, const std::stringstream& ss);

void writeStrStreamMode(const String output_file,
                        const std::stringstream& ss,
                        std::ios_base::openmode mode = std::ios_base::out);

#endif /* include guard */