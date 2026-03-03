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

#define ERROR_STRING(__string_literal__) "ERROR in file [" __FILE__ "] line [" LOG_STRINGIFY___(__LINE__) "]: " __string_literal__