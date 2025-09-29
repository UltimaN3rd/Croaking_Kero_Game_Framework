#include <stdio.h>
#include <time.h>
#include "log.h"

#ifdef NDEBUG
#include "osinterface.h"
#include "folders.h"
#include "game.h"
void log_Init () {
    char output_file[1024];
    snprintf (output_file, sizeof (output_file), "%s/%s/log", os_public.directories.config, GAME_FOLDER_CONFIG);
    folder_CreateDirectoryRecursive (output_file);
	auto t = time(0);
	auto lt = localtime (&t);
    snprintf (output_file, sizeof (output_file), "%s/%s/log/%04d-%02d-%02d-%02d-%02d-%02d", os_public.directories.config, GAME_FOLDER_CONFIG, lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);
    fflush (stderr);
    freopen (output_file, "w", stderr);
}
#else
void log_Init () {}
#endif

void log_Location (const char *file, const int line) {
    fprintf(stderr, "File [%s] Line [%d]: ", file, line);
}

void log_Time () {
	auto t = time(0);
	auto lt = localtime (&t);
    fprintf (stderr, "[%04d-%02d-%02d-%02d-%02d-%02d]", lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);
}