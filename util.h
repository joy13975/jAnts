#ifndef _UTIL_H
#define _UTIL_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {LOG_PRF, LOG_DBG, LOG_WARN, LOG_MSG, LOG_RAW, LOG_ERROR, LOG_DEATH} log_level_t;

#define DEFAULT_LOG_LEVEL LOG_DBG

typedef struct argument_format
{
    const char *short_form;
    const char *long_form;
    const char *description;
} argument_format;

float parse_float(const char *str);
long parse_long(const char *str);
const char *get_error_string();
void print_help_arguement(int indents, const argument_format af);
bool check_arg(const char *arg, const argument_format af);
void _log(const char *filename, int line, log_level_t lvl, char *fmt, ...);

#define CLR_NRM "\x1B[0m"
#define CLR_RED "\x1B[31m"
#define CLR_GRN "\x1B[32m"
#define CLR_YEL "\x1B[33m"
#define CLR_BLU "\x1B[34m"
#define CLR_MAG "\x1B[35m"
#define CLR_CYN "\x1B[36m"
#define CLR_WHT "\x1B[37m"

#define prf(fmt, ...) _log(__FILE__, __LINE__, LOG_PRF, fmt, ##__VA_ARGS__);
#define dbg(fmt, ...) _log(__FILE__, __LINE__, LOG_DBG, fmt, ##__VA_ARGS__);
#define wrn(fmt, ...) _log(__FILE__, __LINE__, LOG_WARN, fmt, ##__VA_ARGS__);
#define err(fmt, ...) _log(__FILE__, __LINE__, LOG_ERROR, fmt, ##__VA_ARGS__);
#define msg(fmt, ...) _log(__FILE__, __LINE__, LOG_MSG, fmt, ##__VA_ARGS__);
#define raw(fmt, ...) _log(__FILE__, __LINE__, LOG_RAW, fmt, ##__VA_ARGS__);
#define die(fmt, ...) _log(__FILE__, __LINE__, LOG_DEATH, fmt, ##__VA_ARGS__);

void set_log_level(log_level_t lvl);
void nsleep(long ns);
double get_timestamp_us();

#endif /* include guard */