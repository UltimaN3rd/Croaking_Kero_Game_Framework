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

#ifndef counted_by
    #ifdef __clang__
    #define counted_by(__count__) __counted_by(__count__)
    #elifdef __GNUC__
    #define counted_by(__count__) __attribute((counted_by(__count__)))
    #endif
#endif

#ifndef unreachable
#if __clang__
    #include <__stddef_unreachable.h>
#elif __GNUC__
    #define unreachable() __builtin_unreachable()
#else
    #error "C23 unreachable() does not seem to be supported by your compiler."
#endif
#endif

#ifndef _Countof
    #if __has_include(<stdcountof.h>)
        #include <stdcountof.h>
    #else
        #define _Countof(__a__) (sizeof (__a__) / sizeof (*__a__))
    #endif
#endif