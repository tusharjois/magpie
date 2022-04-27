#include "logger.h"
#include <stdio.h>
#include <string.h>

static char FMT_BUFFER[LOGGER_MAX];
static int LOGGING_LEVEL = ALL;
static int FORCE_FLUSH = 0;


/* Get logger level (default is INFO) */
int get_logger_level(char* logger_level) {
    if (!strcmp(logger_level, "ALL")) 
        return ALL;
    else if (!strcmp(logger_level, "TRACE")) 
        return TRACE;
    else if (!strcmp(logger_level, "DEBUG")) 
        return DEBUG;
    else if (!strcmp(logger_level, "INFO")) 
        return INFO;
    else if (!strcmp(logger_level, "WARN")) 
        return WARN;
    else if (!strcmp(logger_level, "ERROR")) 
        return ERROR;
    else if (!strcmp(logger_level, "FATAL")) 
        return FATAL;
    else if (!strcmp(logger_level, "OFF")) 
        return OFF;
    else {
        printf("Logger level mismatch - default set to INFO ");
        return INFO;
    }
}

/* Set logging level */
void logger_init(int level, int force_flush) {
    LOGGING_LEVEL = level;
    FORCE_FLUSH = force_flush;
}

/* Decorate output string to be printed */
void set_formatter(char* buffer, const char* fmt, int level) {
    if (level == ALL) {
        sprintf(buffer, "\rALL: %s\n", fmt);
    } else if (level == TRACE) {
        sprintf(buffer, "\rTRACE: %s\n", fmt);
    } else if (level == DEBUG) {
        sprintf(buffer, "\rDEBUG: %s\n", fmt);
    } else if (level == INFO) {
        sprintf(buffer, "\rINFO: %s\n", fmt);
    } else if (level == WARN) {
        sprintf(buffer, "\rWARN: %s\n", fmt);
    } else if (level == ERROR) {
        sprintf(buffer, "\rERROR: %s\n", fmt);
    } else if (level == FATAL) {
        sprintf(buffer, "\rFATAL: %s\n", fmt);
    } else if (level == OFF) {
        sprintf(buffer, "\rOFF: %s\n", fmt);
    } else {
        sprintf(buffer, "\r[%d]: %s\n", level, fmt);
    }
}

/* Log string at a given logging level. */
void logger(int level, const char* fmt, ...) {
    // Collect printf arguments
    va_list args;
    va_start(args, fmt);

    // Print if level is high enough
    if (level >= LOGGING_LEVEL) {
        set_formatter(FMT_BUFFER, fmt, level);
        vfprintf(stdout, FMT_BUFFER, args);
        if (FORCE_FLUSH)
            fflush(stdout);  // 0 = stdout
    }
}