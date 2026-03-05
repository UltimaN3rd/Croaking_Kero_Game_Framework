// This file is #included from ../folders_common.c
#include <stdio.h>
#include <unistd.h>

// If directory is NULL, searches current folder.
folder_FindFirstFile_return_t folder_FindFirstFile (const char *directory) {
    folder_t folder;
    char *dir = folder.name;
    if (directory == NULL) dir = ".";
    else {
        sprintf (dir, directory);
    }

    folder.linux_directory = opendir (dir);
    if (folder.linux_directory == NULL) {
        LOG ("No files found in directory %s", directory);
        return (folder_FindFirstFile_return_t){.is_error = true, .error = {.code = folder_FindFirstFile_error_NoFilesFound, .string = ERROR_STRING("No files found")}};
    }
    folder.linux_entry = readdir (folder.linux_directory);
    if (folder.linux_entry == NULL) {
        LOG ("No files found in directory %s", directory);
        return (folder_FindFirstFile_return_t){.is_error = true, .error = {.code = folder_FindFirstFile_error_NoFilesFound, .string = ERROR_STRING("No files found")}};
    }
    while (folder.linux_entry->d_name[0] == '.') {
        folder.linux_entry = readdir (folder.linux_directory);
        if (folder.linux_entry == NULL) {
            LOG ("No files found in directory %s", directory);
            return (folder_FindFirstFile_return_t){.is_error = true, .error = {.code = folder_FindFirstFile_error_NoFilesFound, .string = ERROR_STRING("No files found")}};
        }
    }
    if (strlen (folder.linux_entry->d_name) > folder_NAME_MAX_LENGTH) {
        LOG ("Directory %s, first file %s filename is too long", directory, folder.linux_entry->d_name);
        return (folder_FindFirstFile_return_t){.is_error = true, .error = {.code = folder_FindFirstFile_error_FilenameTooLong, .string = ERROR_STRING("First file's name is too long")}};
    }
    sprintf (folder.name, folder.linux_entry->d_name);
    folder.is_folder = (folder.linux_entry->d_type == DT_DIR);

    return (folder_FindFirstFile_return_t){.is_error = false, .folder = folder};
}

folder_FindNextFile_return_t folder_FindNextFile (folder_t *folder) {
    folder->linux_entry = readdir (folder->linux_directory);
    if (folder->linux_entry == NULL)
        return (folder_FindNextFile_return_t){.is_error = true, .error = {.code = folder_FindNextFile_error_NoMoreFiles, .string = ERROR_STRING ("There are no more files in this folder.")}};
    while (folder->linux_entry->d_name[0] == '.') {
        folder->linux_entry = readdir (folder->linux_directory);
        if (folder->linux_entry == NULL)
            return (folder_FindNextFile_return_t){.is_error = true, .error = {.code = folder_FindNextFile_error_NoMoreFiles, .string = ERROR_STRING ("There are no more files in this folder.")}};
    }
    if (strlen (folder->linux_entry->d_name) > folder_NAME_MAX_LENGTH) {
        LOG ("Folder name \"%s\" exceeds the max path length %d", folder->linux_entry->d_name, folder_NAME_MAX_LENGTH);
        return (folder_FindNextFile_return_t){.is_error = true, .error = {.code = folder_FindNextFile_error_NameTooLong, .string = ERROR_STRING ("This file's name is too long.")}};
    }
    sprintf (folder->name, folder->linux_entry->d_name);
    folder->is_folder = (folder->linux_entry->d_type == DT_DIR);
    return (folder_FindNextFile_return_t){.is_error = false};
}

void folder_Close (folder_t *folder) {
    closedir (folder->linux_directory);
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
        mkdir (slice_start, 0700);
        if (!end) {
            *slice_end = '/';
            ++slice_end;
        }
    }
}

void folder_CreateDirectory (const char *directory) {
    mkdir (directory, 0700);
}
