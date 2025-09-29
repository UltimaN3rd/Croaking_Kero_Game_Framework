#pragma once

#include <stdio.h>

void log_Init ();
void log_Time ();
void log_Location (const char *file, const int line);

#define LOG(...) do {\
    log_Location (__FILE__, __LINE__);\
    fprintf (stderr, __VA_ARGS__);\
    fprintf (stderr, "\n");\
} while (false)

#define LOG_STRINGIFY___2(x) #x
#define LOG_STRINGIFY___(x) LOG_STRINGIFY___2(x)

#define ERROR_STRING(__string_literal__) "ERROR in file ["__FILE__"] line ["LOG_STRINGIFY___(__LINE__)"]: "__string_literal__