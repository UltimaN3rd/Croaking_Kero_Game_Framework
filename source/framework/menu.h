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

#include <stdint.h>
#include <stddef.h>

typedef struct submenu_t submenu_t;
struct submenu_t {
    const char *name;
    submenu_t *parent;
    int selected;
    bool retain_selection;
    uint8_t color;
    void (*on_exit_func) ();
    enum { menu_type_list, menu_type_explorer, menu_type_name_creator, menu_type_internal } type;
    union {
        struct {
            #define MENU_LIST_MAX_ITEMS 12
            int item_count;
            struct {
                const char *name;
                enum { menu_list_item_type_function, menu_list_item_type_submenu, menu_list_item_type_slider, menu_list_item_type_toggle } type;
                union {
                    void (*Function) ();
                    submenu_t *submenu;
                    struct {
                        void (*Function) (uint8_t output_0_to_max);
                        uint8_t (*InitialValueFunction) ();
                        uint8_t value_0_to_255, max;
                    } slider;
                    struct {
                        bool *var;
                        void (*on_toggle_func) (bool);
                    } toggle;
                };
            } items[MENU_LIST_MAX_ITEMS];
        } list;
        struct {
            const char* (*const base_directory_func) ();
            void (*const selection_func) (const char *const directory, const char *const filename);
            bool show_name_instead_of_directory;
            int maximum_depth;
            #define MENU_EXPLORER_SELECTABLE_FILES   0b01
            #define MENU_EXPLORER_SELECTABLE_FOLDERS 0b10
            uint8_t selectable;
        } explorer;
        struct {
            char *const text_buffer;
            const size_t *const buffer_size; // Amount of characters that can be held +1 for NULL terminator
            void (*const confirm_func) ();
            uint8_t cursor;
        } name_creator;
    };
};

#include "framework.h"
#include "render.h"

typedef struct {
    typeof((render_state_t){}.background.type) background;
    struct { int x, y, w, h, textw, topoffset; } dimensions;
    const char *filename_to_delete;
    submenu_t *submenu;
} menu_t;

typedef struct {
    bool up, down, confirm, cancel, left, right, backspace, delete;
    struct {
        int x, y;
        uint8_t left;
    } mouse;
    char typing[4];
} menu_inputs_t;

vec2i_t menu_ItemDimensions (const char *item_start);
void menu_CalculateDimensions (menu_t *self);
void menu_Update (menu_t *self, menu_inputs_t inputs);
void menu_Render (menu_t *self, int depth);
void menu_Init (menu_t *self);

extern explorer_t menu_explorer;