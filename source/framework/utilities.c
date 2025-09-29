#include "utilities.h"

// Checks the number of characters of the smaller of the two strings
bool StringsAreTheSame (const char *a, const char *b) {
    if (*a == '\0' || *b == '\0') return false;
    do {
        if (*a != *b) return false;
    } while (*(++a) != '\0' && *(++b) != '\0');
    return true;
}

// If one string is longer, only compares the overlapping characters. "Hello" == "hello world!"
bool StringsAreTheSameCaseInsensitive (const char *a, const char *b) {
    if (*a == '\0' || *b == '\0') return false;
    do {
        int offseta = 0, offsetb = 0;
        if (*a >= 'a' && *a <= 'z') offseta = 'A'-'a';
        if (*b >= 'a' && *b <= 'z') offsetb = 'A'-'a';
        if (*a + offseta != *b + offsetb) return false;
    } while (*(++a) != '\0' && *(++b) != '\0');
    return true;
}

bool StringEndsWithCaseInsensitive (const char *a, const char *b) {
    if (*a == '\0' || *b == '\0') return false;
    int a_length = strlen (a);
    int b_length = strlen (b);
    if (a_length > b_length) a += a_length-b_length;
    if (b_length > a_length) b += b_length-a_length;
    do {
        int offseta = 0, offsetb = 0;
        if (*a >= 'a' && *a <= 'z') offseta = 'A'-'a';
        if (*b >= 'a' && *b <= 'z') offsetb = 'A'-'a';
        if (*a + offseta != *b + offsetb) return false;
    } while (*(++a) != '\0' && *(++b) != '\0');
    return true;
}

char *StringFindMatchingCurlyBrace (char *c) {
    if (*c != '{') return NULL;
    for (;;) {
        ++c;
        switch (*c) {
            case '}': return c;
            case '{': return StringFindMatchingCurlyBrace (c);
            case '\0': return NULL;
            default: break;
        }
    }
    __builtin_unreachable ();
}

void BresenhamLine (int x1, int y1, int x2, int y2, void (*PixelFunction) (int x, int y)) {
    int dx = x2 - x1;
    int ax = abs (dx) * 2;
    int sx = dx < 0 ? -1 : 1;
    int dy = y2 - y1;
    int ay = abs (dy) * 2;
    int sy = dy < 0 ? -1 : 1;

    int x = x1;
    int y = y1;
    
    int d;
    if (ax > ay) {
        d = ay - (ax / 2);
        for (;;) {
            PixelFunction (x, y);
            if (x == x2) break;
            if (d >= 0) {
                y += sy;
                d -= ax;
            }
            x += sx;
            d += ay;
        }
    }
    else {
        d = ax - ay / 2;
        for (;;) {
            PixelFunction (x, y);
            if (y == y2) break;
            if (d >= 0) {
                x += sx;
                d -= ay;
            }
            y += sy;
            d += ax;
        }
    }
}