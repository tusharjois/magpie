#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define ALL 0
#define TRACE 5
#define DEBUG 10
#define INFO 20
#define WARN 25
#define ERROR 30
#define FATAL 35
#define OFF 60

#define LOGGER_MAX 2048

int get_logger_level(char* logger_level);

void logger_init(int level, int force_flush);

void logger(int level, const char* formatter, ...);
