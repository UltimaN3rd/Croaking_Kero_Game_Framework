#pragma once

#include <stdint.h>
#include "update.h"

typedef struct {
    int32split_t x, y;
    int16_t time; // If negative, lasts forever (until state change)
    int16_t vx, vy;
    int8_t depth;
    const char *text;
} text_popup_t;

bool TextPopup_Create (const char *const text, int16_t x, int16_t y, int16_t vx, int16_t vy, int16_t time, int8_t depth);
bool TextPopup_Update (text_popup_t *self);
