#include "text_popup.h"

bool TextPopup_Update (text_popup_t *self) {
    self->x.i32 += self->vx;
    self->y.i32 += self->vy;
    Render_Text (.string = self->text, .x = self->x.high, .y = self->y.high, .depth = self->depth, .payload = {self->payload.count, self->payload._});
    // Below logic allows time < 0 to mean infinite time
    if (self->time > 0) --self->time;
    if (self->time == 0) return false;
    return true;
}

bool TextPopup_Create_Loaded (const char *const text, int16_t x, int16_t y, int16_t vx, int16_t vy, int16_t time, int8_t depth, const render_text_payload_t *const payload) {
    const auto payload_count = payload ? Render_TextGetPayloadCountFromString (text) : 0;
    const auto payload_size = sizeof (*(text_popup_t){}.payload._)*payload_count;
    const auto text_size = strlen(text)+1;
    auto obj = Update_ObjectAlloc (sizeof (text_popup_t) + payload_size + text_size);
    if (obj == NULL) return false;

    obj->UpdateAndRender = (Update_Object_Func_t)TextPopup_Update;
    auto mem = (text_popup_t*)Update_ObjectMemOffsetToAddr(obj->memory_offset);

    *mem = (text_popup_t){
        .x = {.high = x},
        .y = {.high = y},
        .vx = vx, .vy = vy,
        .time = time,
        .depth = depth,
        .text = ((char*)mem) + sizeof (text_popup_t) + payload_size,
        .payload = {
            .count = payload_count,
            ._ = (render_text_payload_t*)(((char*)mem) + sizeof (text_popup_t)),
        },
    };

    memcpy ((char*)mem->text, text, text_size);
    memcpy ((render_text_payload_t*)mem->payload._, payload, payload_size);

    return true;
}