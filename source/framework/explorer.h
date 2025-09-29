#pragma once
#include "folders.h"

#define MENU_EXPLORER_FILENAME_BYTES 1024

typedef struct {
	int depth;
	folder_t folder;
	char current_directory_string[512];
	int file_count;
	char *filenames[64];
    bool is_folder[64];
	// #define EXPLORER_FILENAME_DATA_BYTES (1024*2)
    int filename_bytes_available;
	char filename_data[MENU_EXPLORER_FILENAME_BYTES];
} explorer_t;

typedef struct {
    bool is_error;
    struct {
        enum {explorer_Init_error_TooManyFiles, explorer_Init_error_OutOfCharacters, explorer_Init_error_OpenSubDirectoryFailed, explorer_Init_error_InvalidArgument} code;
        const char *string;
    } error;
} explorer_Init_return_t;
explorer_Init_return_t explorer_Init (explorer_t *explorer, int filename_bytes_allocated, const char *base_directory);

typedef struct {
    bool is_error;
    union {
        struct {
            enum {explorer_OpenSubDirectory_error_NoFilesFound, explorer_OpenSubDirectory_error_FilenameTooLong, explorer_OpenSubDirectory_error_InvalidDirectory, explorer_OpenSubDirectory_error_DirectoryNotFound, explorer_OpenSubDirectory_error_OutOfMemory, explorer_OpenSubDirectory_error_TooManyFiles} code;
            const char *string;
        } error;
    };
} explorer_OpenSubDirectory_return_t;
explorer_OpenSubDirectory_return_t explorer_OpenSubDirectory (explorer_t *explorer, const char *sub_directory);

explorer_OpenSubDirectory_return_t explorer_ReloadDirectory (explorer_t *explorer);

bool explorer_DeleteFile (explorer_t *explorer, int index);

bool explorer_GoUpOneDirectory (explorer_t *explorer);