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
