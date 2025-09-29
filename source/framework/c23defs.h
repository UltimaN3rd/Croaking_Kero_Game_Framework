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