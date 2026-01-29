#include "fullscreen_textbox.h"
#include "update.h"
#include "game.h"

extern update_data_t update_data;

static bool FullscreenTextBox_Update (const char *const self) {
    bool retval = true;
    const auto dimensions = font_StringDimensions(&resources_framework_font, self, NULL);
    const int16_t l = (RESOLUTION_WIDTH - dimensions.w) / 2;
    const int16_t r = l + dimensions.w - 1;
    const int16_t b = (RESOLUTION_HEIGHT - dimensions.h) / 2;
    const int16_t t = b + dimensions.h - 1;
    Render_DarkenRectangle(.depth = 127, .l = 0, .r = RESOLUTION_WIDTH-1, .b = 0, .t = RESOLUTION_HEIGHT-1, .levels = 1);
    Render_DarkenRectangle(.depth = 127, .l = l, .r = r, .b = b, .t = t, .levels = 1);
    Render_Shape (.shape = {.type = render_shape_rectangle, .rectangle = {.x = l-1, .y = b-1, .w = dimensions.w+2, .h = dimensions.h+2, .color_edge = 1}}, .depth = 127);
    Render_Text (.string = self, .center_horizontally_on_screen = true, .center_vertically_on_screen = true, .depth = 127);
    if (update_data.frame.mouse.buttons[MOUSE_LEFT] & KEY_PRESSED || update_data.frame.keyboard[os_KEY_ENTER] & KEY_PRESSED || update_data.frame.keyboard[os_KEY_ESCAPE] & KEY_PRESSED || update_data.frame.keyboard[os_KEY_SPACE] & KEY_PRESSED) {
        retval = false; // Delete self
    }
    Update_ClearInputAll();
    return retval;
}

bool FullscreenTextBox_Create (const char *const text) {
    const auto len = strlen(text) + 1; // Add one for null terminator
    char a[len];
    memcpy (a, text, len);
    return Update_ObjectCreate (FullscreenTextBox_Update, 127, true, a);
}
