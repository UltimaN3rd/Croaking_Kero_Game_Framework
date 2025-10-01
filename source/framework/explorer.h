// Copyright [2025] [Nicholas Walton]
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
	char filename_data[MENU_EXPLORER_FILENAME_BYTES];
} explorer_t;

typedef struct {
    bool is_error;
    struct {
        enum {explorer_Init_error_TooManyFiles, explorer_Init_error_OutOfCharacters, explorer_Init_error_OpenSubDirectoryFailed, explorer_Init_error_InvalidArgument} code;
        const char *string;
    } error;
} explorer_Init_return_t;
explorer_Init_return_t explorer_Init (explorer_t *explorer, const char *base_directory);

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