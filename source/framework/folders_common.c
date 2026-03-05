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

#include "folders.h"

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#ifndef FOLDERS_PRINT_ERRORS
#pragma push_macro ("LOG");
#undef LOG
#define LOG(...)
#endif

static char folder_base_directory[32768];

void folder_SetCurrentFolderAsBaseDirectory () {
    getcwd (folder_base_directory, sizeof(folder_base_directory));
}

void folder_ReturnToBaseDirectory () {
    chdir (folder_base_directory);
}

folder_ChangeDirectory_return_t folder_ChangeDirectory (const char *directory) {
    if (chdir (directory) == 0) return (folder_ChangeDirectory_return_t){.is_error = false};
    return (folder_ChangeDirectory_return_t){.is_error = true, .error.string = strerror (errno)};
}

folder_ChangeDirectory_return_t folder_ChangeDirectory_CreateIfNotExist (char *directory) {
    folder_CreateDirectoryRecursive(directory);
    if (chdir (directory) == 0) return (folder_ChangeDirectory_return_t){.is_error = false};
    return (folder_ChangeDirectory_return_t){.is_error = true, .error.string = strerror (errno)};
}

void folder_PrintCurrentDirectory () {
    LOG ("Current directory: %s\n", getcwd(NULL, 0));
}

bool folder_CopyFile (const char *source, const char* destination) {
    bool retval = false;
    FILE *a = NULL, *b = NULL;
    do {
        a = fopen (source, "rb"); if (!a) break;
        b = fopen (destination, "wb"); if (!b) break;
        char buffer[1024];
        size_t read_bytes;
        bool badread = false;
        do {
            read_bytes = fread (buffer, 1, 1024, a);
            size_t written_bytes = fwrite (buffer, 1, read_bytes, b);
            if (written_bytes != read_bytes) {
                badread = true;
                break;
            }
        } while (read_bytes == 1024);
        if (!badread) retval = true;
    } while (false);
    if (a) fclose (a);
    if (b) fclose (b);
    return retval;;
}







#ifdef WIN32
#include "windows/folders.c"
#elif defined __linux__
#include "posix/folders.c"
#elif defined __APPLE__
#include "posix/folders.c"
#endif

#ifndef FOLDERS_PRINT_ERRORS
#pragma pop_macro ("LOG");
#endif