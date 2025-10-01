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

// USAGE
// VAR_DEFER (int, greg, { printf ("greg is falling out of scope now D: greg = %d\n", *this); } ) = 10;
// or
// DEFER (printf ("This code has been deferred!\n"));

#define VAR_DEFER_UNIQUE_NAME__(counter) unique_##counter
#define VAR_DEFER_UNIQUE_NAME_(counter) VAR_DEFER_UNIQUE_NAME__(counter)
#define VAR_DEFER_UNIQUE_NAME VAR_DEFER_UNIQUE_NAME_(__COUNTER__)

#define VAR_DEFER_(type, name, function_body, unique_name) \
void unique_name (type *this) function_body \
type name __attribute__((cleanup(unique_name)))

#define VAR_DEFER(type, name, function_body) VAR_DEFER_(type, name, function_body, VAR_DEFER_UNIQUE_NAME)

#define DEFER(function_body) VAR_DEFER (char, VAR_DEFER_UNIQUE_NAME, { function_body ;})
