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

#include <sys/stat.h>
#include <stdbool.h>
#ifdef FOLDERS_PRINT_ERRORS
#include "log.h"
#endif








#ifdef WIN32

#define folder_NAME_MAX_LENGTH 300

#include <windows.h>

typedef struct {
    HANDLE win32_handle;
    char name[folder_NAME_MAX_LENGTH+1];
    bool is_folder;
} folder_t;

static inline bool folder_DirectoryExists (const char *directory) {
    DWORD attributes = GetFileAttributesA(directory);
    return (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY));
}

static inline bool folder_FileExists (const char *directory) {
    DWORD attributes = GetFileAttributesA(directory);
    return (attributes != INVALID_FILE_ATTRIBUTES);
}





// def WIN32
#elif defined __linux__ || defined __APPLE__

#define folder_NAME_MAX_LENGTH 512

#include <dirent.h>

typedef struct {
    DIR *linux_directory;
    struct dirent *linux_entry;
    char name[folder_NAME_MAX_LENGTH+1];
    bool is_folder;
} folder_t;

static inline bool folder_DirectoryExists (const char *directory) {
    DIR *dir = opendir (directory);
    if (dir) {
        closedir (dir);
        return true;
    }
    else return false;
}

static inline bool folder_FileExists (const char *directory) {
    struct stat stat_buffer;
    if (stat (directory, &stat_buffer) == 0) return true;
    else return false;
}
// defined __linux__ || defined __APPLE__
#endif






void folder_Close (folder_t *folder);

typedef struct {
    bool is_error;
    struct {
        enum {folder_FindNextFile_error_NoMoreFiles, folder_FindNextFile_error_NameTooLong} code;
        const char *string;
    } error;
} folder_FindNextFile_return_t;
folder_FindNextFile_return_t folder_FindNextFile (folder_t *folder);

typedef struct {
    bool is_error;
    union {
        struct {
            enum {folder_FindFirstFile_error_FilenameTooLong, folder_FindFirstFile_error_NoFilesFound, folder_FindFirstFile_error_InvalidDirectory} code;
            const char *string;
        } error;
        folder_t folder;
    };
} folder_FindFirstFile_return_t;
folder_FindFirstFile_return_t folder_FindFirstFile (const char *directory);

typedef struct {
    bool is_error;
    struct {
        const char *string;
    } error;
} folder_ChangeDirectory_return_t;
folder_ChangeDirectory_return_t folder_ChangeDirectory (const char *directory);
folder_ChangeDirectory_return_t folder_ChangeDirectory_CreateIfNotExist (char *directory);
void folder_SetCurrentFolderAsBaseDirectory ();
void folder_ReturnToBaseDirectory ();
void folder_PrintCurrentDirectory ();

void folder_CreateDirectoryRecursive (char *directory);
void folder_CreateDirectory (const char *directory);
bool folder_CopyFile (const char *source, const char* destination);

#include <stdio.h>
static inline bool folder_DeleteFile (const char *filename) {
    return (remove (filename) == 0);
}