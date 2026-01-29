#pragma once

#include <stdint.h>
#include "update.h"

typedef struct {
    int32split_t x, y;
    int16_t time; // If negative, lasts forever (until state change)
    int16_t vx, vy;
    int8_t depth;
    int16_t text_offset;
    const char *text;
    struct {
        int16_t count;
        const render_text_payload_t *_;
    } payload;
} text_popup_t;


// DANGER!!! When passing a payload pointer, the pointer is what is cached in this object. Make sure the pointer you pass is valid for the lifetime of this object! Perhaps a static object... Perhaps I should change the Update_Object API to allow manual allocation of object size in order to allow you to assemble objects a bit more dynamically?
bool TextPopup_Create_Loaded (const char *const text, int16_t x, int16_t y, int16_t vx, int16_t vy, int16_t time, int8_t depth, const render_text_payload_t *const payload);
static inline bool TextPopup_Create (const char *const text, int16_t x, int16_t y, int16_t vx, int16_t vy, int16_t time, int8_t depth) {
    return TextPopup_Create_Loaded (text, x, y, vx, vy, time, depth, NULL);
}
bool TextPopup_Update (text_popup_t *self);
