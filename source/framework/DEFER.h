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
// defer {
//     printf ("This code has been deferred!\n");
// }

#if __has_include(<stddefer.h>)
# if defined(__clang__) && !defined(__STDC_DEFER_TS25755__)
#  define __STDC_DEFER_TS25755__
# endif
# include <stddefer.h>
#elif __GNUC__ > 8
# define defer _Defer
# define _Defer      _Defer_A(__COUNTER__)
# define _Defer_A(N) _Defer_B(N)
# define _Defer_B(N) _Defer_C(_Defer_func_ ## N, _Defer_var_ ## N)
# define _Defer_C(F, V)                                                 \
  auto void F(int*);                                                    \
  __attribute__((__cleanup__(F), __deprecated__, __unused__))           \
     int V;                                                             \
  __attribute__((__always_inline__, __deprecated__, __unused__))        \
    inline auto void F(__attribute__((__unused__)) int*V)
#else
# error "The _Defer feature seems unavailable. Compile with either GCC version 8 or later, or Clang version 22 or later."
#endif
