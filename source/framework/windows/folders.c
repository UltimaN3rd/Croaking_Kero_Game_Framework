// Link with Shlwapi.lib for PathIsDirectoryA
// This file is #included from ../folders.c

#include <windows.h>
#include <shlwapi.h>
#include <stdio.h>

void folder_Close (folder_t *folder) {
    FindClose (folder->win32_handle);
}


folder_FindNextFile_return_t folder_FindNextFile (folder_t *folder) {
    WIN32_FIND_DATAA data;
    if (FindNextFileA (folder->win32_handle, &data) == 0) {
        return (folder_FindNextFile_return_t){.is_error = true, .error = {.code = folder_FindNextFile_error_NoMoreFiles, .string = ERROR_STRING ("There are no more files in this folder.")}};
    }
    while (data.cFileName[0] == '.') {
        if (FindNextFileA (folder->win32_handle, &data) == 0) return (folder_FindNextFile_return_t){.is_error = true, .error = {.code = folder_FindNextFile_error_NoMoreFiles, .string = ERROR_STRING ("There are no more files in this folder.")}};
    }
    if (strlen (data.cFileName) > folder_NAME_MAX_LENGTH) {
        LOG ("Folder name \"%s\" exceeds the max path length %d", data.cFileName, folder_NAME_MAX_LENGTH);
        return (folder_FindNextFile_return_t){.is_error = true, .error = {.code = folder_FindNextFile_error_NameTooLong, .string = ERROR_STRING ("This file's name is too long.")}};
    }
    sprintf (folder->name, data.cFileName);
    folder->is_folder = data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
    return (folder_FindNextFile_return_t){.is_error = false};
}

// If directory is NULL, searches current folder.

folder_FindFirstFile_return_t folder_FindFirstFile (const char *directory) {
    folder_t folder;
    char *dir = folder.name;
    int length = strlen (directory);
    if (length > folder_NAME_MAX_LENGTH) {
        LOG ("Directory \"%s\" is too long", directory);
        return (folder_FindFirstFile_return_t){.is_error = true, .error = {.code = folder_FindFirstFile_error_InvalidDirectory, .string = ERROR_STRING("Directory is too long")}};
    }
    sprintf (dir, directory);
    if (directory == NULL || length == 0) dir[0] = '*';
    else if (length == 1) {// Either a one-letter folder or already *
        if (dir[0] == '/') { dir[1] = '*'; dir[2] = '\0'; } // Root folder
        else if (dir[0] != '*') { dir[1] = '/'; dir[2] = '*'; dir[4] = '\0'; }
    }
    else { // Fix up directory string. Make sure it ends in /*
        if (length <= folder_NAME_MAX_LENGTH) {
            sprintf (dir, directory);
        }
        if (dir[length-1] == '*') { // Last character is a *
            if (dir[length-2] == '\\' || dir[length-2] == '/') {
                // String is already conditioned
            }
            else {
                LOG ("Directory \"%s\" is invalid. the \'*\' character can only be used when preceded and followed by a /, like: \"/*/\"", directory);
                return (folder_FindFirstFile_return_t){.is_error = true, .error = {.code = folder_FindFirstFile_error_InvalidDirectory, .string = ERROR_STRING("Directory invalid. The * character can only be used with a preceding and following /, like: \"/*/\"")}};
            }
        }
        else if (dir[length-1] == '/' || dir[length-1] == '\\') { // Last character is a slash
            if (length >= 3 && dir[length-2] == '*') { // Ends in */
                if (dir[length-3] == '/' || dir[length-3] == '\\') { // Ends in /*/, which is valid
                }
                else {
                    LOG ("Directory \"%s\" is invalid. the \'*\' character can only be used when preceded and followed by a /, like: \"/*/\"", directory);
                    return (folder_FindFirstFile_return_t){.is_error = true, .error = {.code = folder_FindFirstFile_error_InvalidDirectory, .string = ERROR_STRING("Directory invalid. The * character can only be used with a preceding and following /, like: \"/*/\"")}};
                }
            }
            else if (length < folder_NAME_MAX_LENGTH) {
                dir[length] = '*';
                dir[length+1] = '\0';
            }
            else {
                LOG ("Directory \"%s\" exceeds the max path length %d after adding *", directory, folder_NAME_MAX_LENGTH);
                return (folder_FindFirstFile_return_t){.is_error = true, .error = {.code = folder_FindFirstFile_error_InvalidDirectory, .string = ERROR_STRING("This directory name is too long after adding *")}};
            }
        }
        else { // Last character is neither a * or /
            if (length < folder_NAME_MAX_LENGTH - 2) {
                dir[length] = '/';
                dir[length+1] = '*';
                dir[length+2] = '\0';
            }
            else {
                LOG ("Directory \"%s\" exceeds the max path length %d after adding /*", directory, folder_NAME_MAX_LENGTH);
                return (folder_FindFirstFile_return_t){.is_error = true, .error = {.code = folder_FindFirstFile_error_InvalidDirectory, .string = ERROR_STRING("This directory name is too long after adding /*")}};
            }
        }
    }

    WIN32_FIND_DATAA data;
    HANDLE handle = FindFirstFileA (dir, &data);
    if (handle == INVALID_HANDLE_VALUE) {
        LOG ("No files found in directory %s", directory);
        return (folder_FindFirstFile_return_t){.is_error = true, .error = {.code = folder_FindFirstFile_error_NoFilesFound, .string = ERROR_STRING("No files found")}};
    }
    folder.win32_handle = handle;
    if (strlen (data.cFileName) > folder_NAME_MAX_LENGTH) {
        folder_Close (&folder);
        LOG ("Directory %s, first file %s filename is too long", directory, data.cFileName);
        return (folder_FindFirstFile_return_t){.is_error = true, .error = {.code = folder_FindFirstFile_error_FilenameTooLong, .string = ERROR_STRING("First file's name is too long")}};
    }
    sprintf (folder.name, data.cFileName);
    folder.is_folder = data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
    while (folder.name[0] == '.') { // Skip the '.' and '..' listings
        auto result = folder_FindNextFile (&folder);
        if (result.is_error) {
            folder_Close (&folder);
            LOG ("Directory %s contains no files", directory);
            return (folder_FindFirstFile_return_t){.is_error = true, .error = {.code = folder_FindFirstFile_error_NoFilesFound, .string = ERROR_STRING("Directory contains no files")}};
        }
    }

    return (folder_FindFirstFile_return_t){.is_error = false, .folder = folder};
}

void folder_CreateDirectoryRecursive (char *directory) {
    // need to make individual levels rather than all at once, which doesn't work. IE
    // stuff/apples/seeds must be done as stuff, then stuff/apples, then stuff/apples/seeds
    if (*directory == 0) return;
    char *slice_start, *slice_end;
    slice_start = slice_end = directory;
    bool end = false;
    while (!end) {
        while (*slice_end != 0 && *slice_end != '/' && *slice_end != '\\') ++slice_end;
        if (*slice_end == 0) end = true;
        *slice_end = 0;
        mkdir (slice_start);
        if (!end) {
            *slice_end = '/';
            ++slice_end;
        }
    }
}

void folder_CreateDirectory (const char *directory) {
    mkdir (directory);
}
