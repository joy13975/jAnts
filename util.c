#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>

#include "util.h"

log_level_t log_level = DEFAULT_LOG_LEVEL;

float parse_float(const char *str)
{
    char *next;
    float val = strtof(str, &next);
    if (strlen(next))
        die("Failed to parse float from \"%s\"\n", str);
    return val;
}

long parse_long(const char *str)
{
    char *next;
    long val = strtol(str, &next, 10);
    if (strlen(next))
        die("Failed to parse long integer from \"%s\"\n", str);

    return val;
}

const char *get_error_string()
{
    return strerror(errno);
}

void print_help_arguement(int indents, const argument_format af)
{
    for (int i = 0; i < indents; i++)
        raw("    ");

    raw("%s, %s\n", af.short_form, af.long_form);

    for (int i = 0; i < indents + 1; i++)
        raw("    ");
    raw("%s\n", af.description);
}

bool check_arg(const char *arg, const argument_format af)
{
    return !strcmp(arg, af.short_form) || !strcmp(arg, af.long_form);
}

void set_log_level(log_level_t lvl)
{
    if (lvl < 0 || lvl >= LOG_ERROR)
    {
        err("Invalid log level: %d\n", lvl);
        err("Must be between 0 and %d\n", LOG_ERROR);
        raw("   LOG_PRF    : %d\n", LOG_PRF);
        raw("   LOG_DBG    : %d\n", LOG_DBG);
        raw("   LOG_WARN   : %d\n", LOG_WARN);
        raw("   LOG_MSG    : %d\n", LOG_MSG);
        raw("   LOG_RAW    : %d\n", LOG_RAW)
        // raw("   LOG_ERROR  : %d\n", LOG_ERROR);
        // raw("   LOG_DEATH  : %d\n", LOG_DEATH);
        exit(1);
    }
    log_level = lvl;
    msg("Debug level set to: %d\n", log_level);
}

void _log(const char *filename, int line, log_level_t lvl, char *fmt, ...)
{
    if (lvl >= log_level) {
        va_list args;
        va_start(args, fmt);

        char *new_fmt;
        FILE *fd;
        switch (lvl)
        {
        case LOG_PRF:
            fd = stdout;
            asprintf(&new_fmt, "%s[PRF]%s %s", CLR_CYN, CLR_NRM, fmt);
            break;
        case LOG_DBG:
            fd = stdout;
            asprintf(&new_fmt, "%s[DBG]%s %s", CLR_BLU, CLR_NRM, fmt);
            break;
        case LOG_WARN:
            fd = stdout;
            asprintf(&new_fmt, "%s[WRN]%s %s", CLR_YEL, CLR_NRM, fmt);
            break;
        case LOG_MSG:
            fd = stdout;
            asprintf(&new_fmt, "%s[MSG]%s %s", CLR_MAG, CLR_NRM, fmt);
            break;
        case LOG_RAW:
            fd = stdout;
            new_fmt = fmt;
            break;
        case LOG_ERROR:
            fd = stderr;
            asprintf(&new_fmt, "%s[ERR]%s %s", CLR_RED, CLR_NRM, fmt);
            break;
        case LOG_DEATH:
            fd = stderr;
            asprintf(&new_fmt, "%s[DIE %s:%d]%s %s\n", CLR_RED, filename, line, CLR_NRM, fmt);
            break;
        default:
            fd = stdout;
            asprintf(&new_fmt, "%s[???]%s %s", CLR_RED, CLR_NRM, fmt);
        }

        vfprintf(fd, new_fmt, args);
        fflush(fd);

        va_end(args);

        if (lvl == LOG_DEATH)
        {
            if (errno != 0)
                fprintf(fd, "\nError before death: \"%s\"\n", get_error_string);
            exit(1);
        }
    }
}

void nsleep(long ns)
{
    struct timespec sleep_time;
    sleep_time.tv_sec = ns / 1000000000;
    sleep_time.tv_nsec = ns % 1000000000;
    nanosleep(&sleep_time, NULL);
}

double get_timestamp_us()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_usec + tv.tv_sec * 1e6;
}