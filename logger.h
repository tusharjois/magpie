#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* Print information if level >= LEVEL
For submission and benchmarking, we set the log level to 20 (INFO)
*/
#define ALL 0
#define TRACE 5
#define DEBUG 10
#define INFO 20
#define OUTPUT 25
#define WARN 30
#define ERROR 40
#define FATAL 50
#define OFF 60

#define LOGGER_MAX 2048

void logger_init(int level, int force_flush);

void logger(int level, const char* formatter, ...);
