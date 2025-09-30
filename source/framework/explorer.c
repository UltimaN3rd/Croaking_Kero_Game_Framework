#include "explorer.h"

#include <string.h>
#include <inttypes.h>
#include "utilities.h"

#ifndef EXPLORER_PRINT_ERRORS
#pragma push_macro ("LOG");
#undef LOG
#define LOG(...)
#endif

// Change the below function into general "OpenDirectory" or "OpenSubDirectory", and create a new initialization function which calls it after the basic setup to open the base folder
explorer_Init_return_t explorer_Init (explorer_t *explorer, const char *base_directory) {
    if (!explorer) {
        LOG ("Trying to load directory %s. The explorer pointer was NULL D:", base_directory);
        return (explorer_Init_return_t){.is_error = true, .error = {.code = explorer_Init_error_InvalidArgument, .string = ERROR_STRING("explorer was a NULL pointer. You are very naughty! >:(")}};
    }
	explorer->depth = -1;
	sprintf (explorer->current_directory_string, base_directory);
    auto result = explorer_OpenSubDirectory (explorer, "/");
    if (result.is_error) {
        LOG ("Trying to load directory %s. Failed to open the directory. explorer_OpenSubDirectory experienced this error: %s", base_directory, result.error.string);
        return (explorer_Init_return_t){.is_error = true, .error = {.code = explorer_Init_error_OpenSubDirectoryFailed, .string = result.error.string}};
    }
    return (explorer_Init_return_t){.is_error = false};
}

bool explorer_DeleteFile (explorer_t *explorer, int index) {
    char full_directory[1025];
    snprintf (full_directory, 1024, "%s%s", explorer->current_directory_string, explorer->filenames[index]);
    if (remove (full_directory) != 0) {
        LOG ("In directory \"%s\" trying to delete file[%d] named %s. File not found", explorer->current_directory_string, index, explorer->filenames[index]);
        return false;
    }
    return true;
}

explorer_OpenSubDirectory_return_t explorer_ReloadDirectory (explorer_t *explorer) {
    __label__ skip_separating_files_and_folders, skip_alphabetizing_folders, skip_alphabetizing_files;

	{auto result = folder_FindFirstFile (explorer->current_directory_string);
	if (result.is_error) {
		explorer->file_count = 0;
        explorer->filename_data[0] = '\0';
        explorer->filenames[0] = explorer->filename_data;
        explorer->is_folder[0] = false;
        switch (result.error.code) {
            case folder_FindFirstFile_error_NoFilesFound:
                LOG ("Opening sub-folder \"%s\" No files found", explorer->current_directory_string);
                return (explorer_OpenSubDirectory_return_t){.is_error = true, .error = {.code = explorer_OpenSubDirectory_error_NoFilesFound, .string = result.error.string}};
            case folder_FindFirstFile_error_FilenameTooLong:
                LOG ("Opening sub-folder \"%s\" Filename too long", explorer->current_directory_string);
                return (explorer_OpenSubDirectory_return_t){.is_error = true, .error = {.code = explorer_OpenSubDirectory_error_FilenameTooLong, .string = result.error.string}};
            case folder_FindFirstFile_error_InvalidDirectory:
                LOG ("Opening sub-folder \"%s\" Invalid directory", explorer->current_directory_string);
                return (explorer_OpenSubDirectory_return_t){.is_error = true, .error = {.code = explorer_OpenSubDirectory_error_InvalidDirectory, .string = result.error.string}};
        }
	}
    explorer->folder = result.folder;}

	explorer->filenames[0] = explorer->filename_data;
    explorer->is_folder[0] = explorer->folder.is_folder;
	sprintf (explorer->filename_data, explorer->folder.name);
	explorer->file_count = 1;
	bool too_many_files = false;
	bool out_of_characters = false;
    bool found_at_least_one_folder = explorer->folder.is_folder;
    bool found_at_least_one_file = !explorer->folder.is_folder;
    auto result = folder_FindNextFile (&explorer->folder);
	while (!result.is_error) {
		explorer->filenames[explorer->file_count] = &explorer->filenames[explorer->file_count-1][strlen(explorer->filenames[explorer->file_count-1])+1];
        if (explorer->is_folder[explorer->file_count-1]) ++explorer->filenames[explorer->file_count];
		if ((explorer->filenames[explorer->file_count] + strlen (explorer->folder.name) + (explorer->folder.is_folder ? 1 : 0)) - explorer->filename_data > MENU_EXPLORER_FILENAME_BYTES) {
            LOG ("explorer filename [%s] is too long [%"PRId64"] > [%d]", explorer->folder.name, (int64_t)((explorer->filenames[explorer->file_count] + strlen (explorer->folder.name) + 1) - explorer->filename_data), MENU_EXPLORER_FILENAME_BYTES);
			out_of_characters = true;
			break;
		}
        sprintf (explorer->filenames[explorer->file_count], explorer->folder.name);
        if (explorer->folder.is_folder) {
            explorer->is_folder[explorer->file_count] = true;
            found_at_least_one_folder = true;
        }
        else {
            explorer->is_folder[explorer->file_count] = false;
            found_at_least_one_file = true;
        }
		if (++explorer->file_count > 64) {
			explorer->file_count = 64;
            too_many_files = true;
			break;
		}
        result = folder_FindNextFile (&explorer->folder);
	}

	folder_Close (&explorer->folder);
	if (out_of_characters) {
        LOG ("Opening sub-folder \"%s\" Out of memory", explorer->folder.name);
        return (explorer_OpenSubDirectory_return_t){.is_error = true, .error = {.code = explorer_OpenSubDirectory_error_OutOfMemory, .string = ERROR_STRING("Out of memory")}};
    }

    // Sorting
    int left = 0, right = explorer->file_count-1;
    if (!found_at_least_one_file || !found_at_least_one_folder) goto skip_separating_files_and_folders;

    // Put all the folders first, then files
    while (!explorer->is_folder[left]) ++left; // Find the left-most folder
    char *t;
    if (left != 0) {
        t = explorer->filenames[0]; explorer->filenames[0] = explorer->filenames[left]; explorer->filenames[left] = t; // Move the left-most folder to position 0 so the below algorithm terminates without needing extra bounds checks
        explorer->is_folder[0] = true; explorer->is_folder[left] = false;
    }
    left = 1;
    while (explorer->is_folder[right]) --right; // Find the right-most file
    if (right != explorer->file_count-1) {
        t = explorer->filenames[explorer->file_count-1]; explorer->filenames[explorer->file_count-1] = explorer->filenames[right]; explorer->filenames[right] = t; // Move the right-most file to the end position so the below algorithm terminates without needing extra bounds checks
        explorer->is_folder[right] = true; explorer->is_folder[explorer->file_count-1] = false;
    }
    right = explorer->file_count-2;
    do {
        while (explorer->is_folder[left]) ++left;
        while (!explorer->is_folder[right]) --right;
        if (right <= left) break;
        char *t = explorer->filenames[left]; explorer->filenames[left] = explorer->filenames[right]; explorer->filenames[right] = t;
        explorer->is_folder[left] = true; explorer->is_folder[right] = false;
        ++left; --right;
    } while (true);

skip_separating_files_and_folders:

    // Now alphabetize both folders and files separately

    if (!found_at_least_one_folder) goto skip_alphabetizing_folders;
    // Alphabetize folders
    left = 0; right = explorer->file_count-1;
    while (!explorer->is_folder[right]) --right;
    for (int selector = 1; selector <= right; ++selector) {
        int i = selector;
        while (i > 0 && StringCompareCaseInsensitive(explorer->filenames[i], explorer->filenames[i-1]) == -1) {
            char *t = explorer->filenames[i]; explorer->filenames[i] = explorer->filenames[i-1]; explorer->filenames[i-1] = t;
            --i;
        }
    }
    for (int i = 0; i <= right; ++i) {
        char *c = &explorer->filenames[i][strlen(explorer->filenames[i])];
        *(c++) = '/';
        *c = '\0';
    }

skip_alphabetizing_folders:

    if (!found_at_least_one_file) goto skip_alphabetizing_files;
    // Alphabetize files
    left = 0;
    while (explorer->is_folder[left]) ++left;
    right = explorer->file_count-1;
    for (int selector = left + 1; selector <= right; ++selector) {
        int i = selector;
        while (i > left && StringCompareCaseInsensitive(explorer->filenames[i], explorer->filenames[i-1]) == -1) {
            char *t = explorer->filenames[i]; explorer->filenames[i] = explorer->filenames[i-1]; explorer->filenames[i-1] = t;
            --i;
        }
    }

skip_alphabetizing_files:

	if (too_many_files) {
        LOG ("Opening sub-folder \"%s\" Too many files", explorer->folder.name);
        return (explorer_OpenSubDirectory_return_t){.is_error = true, .error = {.code = explorer_OpenSubDirectory_error_TooManyFiles, .string = ERROR_STRING("Too many files")}};
    }
    return (explorer_OpenSubDirectory_return_t){.is_error = false};
}

explorer_OpenSubDirectory_return_t explorer_OpenSubDirectory (explorer_t *explorer, const char *sub_directory) {
    char full_directory[1024];
    snprintf (full_directory, 1024, "%s%s", explorer->current_directory_string, sub_directory);

    if (!folder_DirectoryExists (full_directory)) {
        LOG ("Opening sub-folder \"%s\" Directory not found", full_directory);
        return (explorer_OpenSubDirectory_return_t){.is_error = true, .error = {.code = explorer_OpenSubDirectory_error_DirectoryNotFound, .string = ERROR_STRING("Directory not found")}};
    }

    char previous_directory[512];
    sprintf (previous_directory, explorer->current_directory_string);
	++explorer->depth;
	snprintf (explorer->current_directory_string, 511, full_directory);

    {auto result = explorer_ReloadDirectory (explorer);
    if (result.is_error) {
        switch (result.error.code) {
            case explorer_OpenSubDirectory_error_NoFilesFound:
            case explorer_OpenSubDirectory_error_FilenameTooLong:
            case explorer_OpenSubDirectory_error_InvalidDirectory:
            case explorer_OpenSubDirectory_error_DirectoryNotFound:
            case explorer_OpenSubDirectory_error_OutOfMemory:
            case explorer_OpenSubDirectory_error_TooManyFiles:
                --explorer->depth;
                sprintf (explorer->current_directory_string, previous_directory);
                return explorer_ReloadDirectory (explorer);
        }
    }}
    return (explorer_OpenSubDirectory_return_t){.is_error = false};
}

bool explorer_GoUpOneDirectory (explorer_t *explorer) {
    if (explorer->depth == 0) return false;
    explorer->depth -= 2; // The OpenSubDirectory will increment this again
    // current directory string always ends in a slash, so go back to the second-to-last slash and set it to '\0'. Then enter the subdirectory "/" in order to reopen the folder one level up.
    char *c = &explorer->current_directory_string[strlen(explorer->current_directory_string)-2];
    while (*c != '/' && *c != '\\') --c;
    *c = '\0';
    explorer_OpenSubDirectory (explorer, "/");
    return true;
}

#ifndef EXPLORER_PRINT_ERRORS
#pragma pop_macro ("LOG");
#endif