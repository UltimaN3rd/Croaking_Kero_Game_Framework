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

#include "menu.h"
#include "resources.h"

explorer_t menu_explorer;

// Slider width include one pixel outline - inner width is this width - 2
#define EXPLORER_ITEMS_TO_SHOW 17
#define SLIDER_WIDTH 34
#define SLIDER_MARGIN 2
#define XMARGIN 2
#define TOGGLE_WIDTH (resources_framework_font.line_height - 2)
#define MENU_NAME_VERTICAL_DISTANCE_ABOVE 20
#define MENU_NAME_DISTANCE_BELOW_TOP 20
#define MENU_DISTANCE_ABOVE_BOTTOM 20

static struct { int l, b, r, t; } title_bounds = {};

static struct { int x, y; } mouse;

static uint8_t explorer_scroll = 0;

static void menu_Delete_Yes ();

static bool delete_confirmed = false;
static int delete_confirmation_explorer_file_index = 0;
static const char *delete_confirmation_filename = NULL;
static vec2i_t delete_confirmation_text_offset = {};
static submenu_t submenu_delete_confirmation = {
	.name = "Delete confirmation",
	.color = 131,
	.type = menu_type_internal,
	.list = {
		.item_count = 2,
		.items = {
			{.name = "Delete", .type = menu_list_item_type_function, .Function = menu_Delete_Yes},
			{.name = "Cancel", menu_list_item_type_submenu, .submenu = NULL},
		}
	}
};

void submenu_Init (submenu_t *self) {
	assert (self);
	if (!self->retain_selection) self->selected = 0;
	switch (self->type) {
		case menu_type_explorer: {
			explorer_Init (&menu_explorer, self->explorer.base_directory_func());
			explorer_scroll = 0;
		} break;
		case menu_type_list: {
			for (int i = 0; i < self->list.item_count; ++i) {
				switch (self->list.items[i].type) {
					case menu_list_item_type_submenu:
					case menu_list_item_type_function:
					case menu_list_item_type_toggle: break;
					case menu_list_item_type_slider: {
						self->list.items[i].slider.value_0_to_255 = self->list.items[i].slider.InitialValueFunction ();
						if (self->list.items[i].slider.value_0_to_255 > self->list.items[i].slider.max)
							self->list.items[i].slider.value_0_to_255 = self->list.items[i].slider.max;
					} break;
				}
			}
		} break;
		case menu_type_name_creator: {
			memset (self->name_creator.text_buffer, 0, sizeof (self->name_creator.buffer_size));
			self->name_creator.cursor = 0;
		} break;
		case menu_type_internal:
			unreachable();
	}
}

void menu_Init (menu_t *self, submenu_t *initial_submenu) {
	self->submenu = initial_submenu;
	initial_submenu->parent = NULL;
	submenu_Init (initial_submenu);
	menu_CalculateDimensions(self);
	if (self->transparent_background_darkess && self->selection_color == 0) self->selection_color = 255;
	if (self->selection_color == 0) self->selection_color = 1;
}

vec2i_t menu_ItemDimensions_Length (const char *item_start, const size_t length) {
	const char *item = item_start;
	char c = *item;
	int x = 0, y = 0;
	int width = 0;
	int bottom = 0, top = 0;
	for (int j = 0; j < length && c != '\0'; ++j) {
        switch (c) {
            case ' ': {
				x += resources_framework_font.space_width;
				if (x > width) width = x;
			} break;

            case '\n': {
				const char *spaces = item;
				while (spaces > item_start && *(spaces-1) == ' ') {
					x -= resources_framework_font.space_width;
					--spaces;
				}
				if (x > width) width = x;
                x = 0;
				y -= resources_framework_font.line_height;
            } break;

            default: {
                int i = c - BITMAP_FONT_FIRST_VISIBLE_CHAR;
                if (i >= 0 && i < BITMAP_FONT_NUM_VISIBLE_CHARS) {
                    x += resources_framework_font.bitmaps[i]->w;
                    if (x > width) width = x;
					int b = y;// - resources_framework_font.descent[i]; // Previously measured descent, but decided that the baseline should be used instead. Particularly caused issues when the first item contained a latter below the basline
					int t = b + resources_framework_font.bitmaps[i]->h-1;
					if (b < bottom) bottom = b;
					if (t > top) top = t;
                } // If character wasn't in the range, then we ignore it.
            } break;
        }
        c = *++item;
	}

	return (vec2i_t){.w = width, .h = top - bottom + 1};
}

vec2i_t menu_ItemDimensions (const char *item_start) {
	return menu_ItemDimensions_Length(item_start, strlen (item_start));
}

void menu_CalculateDimensions (menu_t *self) {
	__label__ goto_calculate_dimensions_list;
	self->dimensions = (typeof(self->dimensions)){};
    auto submenu = self->submenu;

	int explorer_start = 0;
	int explorer_end = 0;

	int top_item_offset = 0;

	int y = 0;
	switch (submenu->type) {
		case menu_type_list: {
		goto_calculate_dimensions_list:
			auto item = submenu->list.items;
			bool contains_slider = false, contains_toggle = false;
			repeat (submenu->list.item_count) {
				auto dimensions = menu_ItemDimensions (item->name);
				if (item == submenu->list.items)// first item
					top_item_offset = resources_framework_font.line_height - dimensions.y;
				if (dimensions.width > self->dimensions.w) self->dimensions.w = dimensions.width;
				y -= resources_framework_font.line_height;
				switch (item->type) {
					case menu_list_item_type_function:
					case menu_list_item_type_submenu: break;
					case menu_list_item_type_slider: contains_slider = true; break;
					case menu_list_item_type_toggle: contains_toggle = true; break;
				}
				++item;
			}
			self->dimensions.textw = self->dimensions.w;
			if (contains_slider) self->dimensions.w = MAX (self->dimensions.w, self->dimensions.w + SLIDER_WIDTH + SLIDER_MARGIN * 2);
			if (contains_toggle) self->dimensions.w = MAX (self->dimensions.w, self->dimensions.w + TOGGLE_WIDTH + SLIDER_MARGIN * 2);
			self->dimensions.x = (RESOLUTION_WIDTH - self->dimensions.w) / 2;
			self->dimensions.h = abs(y);
			self->dimensions.y = (RESOLUTION_HEIGHT - self->dimensions.h)/2;
		} break;

		case menu_type_explorer: {
			int file_count = menu_explorer.file_count;
			explorer_start = 0;
			explorer_end = file_count;
			if (file_count > 17) {
				explorer_start = submenu->selected - 8;
				if (explorer_start < 0) explorer_start = 0;
				explorer_end = explorer_start + 17;
				if (explorer_end > file_count) {
					explorer_end = file_count;
					explorer_start = explorer_end - 17;
				}
			}
			for (int i = explorer_start; i < explorer_end; ++i) {
				auto dimensions = menu_ItemDimensions (menu_explorer.filenames[i]);
				if (i == explorer_start)// first item
					top_item_offset = resources_framework_font.line_height - dimensions.y;
				if (dimensions.width > self->dimensions.w) self->dimensions.w = dimensions.width;
				y -= resources_framework_font.line_height;
			}
			self->dimensions.x = 20;
			self->dimensions.h = abs(y);
			self->dimensions.y = (RESOLUTION_HEIGHT - self->dimensions.h)/2;
			// menu->dimensions.y = RESOLUTION_HEIGHT - 10;
			// auto title_dimensions = menu_ItemDimensions(submenu->explorer.show_name_instead_of_directory ? submenu->name : menu_explorer.current_directory_string);
			// title_bounds.l = self->dimensions.x;
			// title_bounds.t = self->dimensions.y + self->dimensions.h-1 + 20;
			// title_bounds.r = title_bounds.l + 1 + title_dimensions.x;
			// title_bounds.b = title_bounds.t - 1 - title_dimensions.y;
			title_bounds.r = RESOLUTION_WIDTH-1;
			title_bounds.l = title_bounds.r - resources_framework_menu_folder_open.w + 1;
			title_bounds.t = RESOLUTION_HEIGHT-1;
			title_bounds.b = title_bounds.t - resources_framework_menu_folder_open.h + 1;
		} break;

		case menu_type_name_creator: {
			auto dimensions = menu_ItemDimensions(submenu->name_creator.text_buffer);
			self->dimensions.x = (RESOLUTION_WIDTH - dimensions.x) / 2;
			self->dimensions.y = (RESOLUTION_HEIGHT - dimensions.y) / 2;
			self->dimensions.w = dimensions.w;
			self->dimensions.h = dimensions.h;
		} break;

		case menu_type_internal: {
			assert (submenu == &submenu_delete_confirmation);
			goto goto_calculate_dimensions_list;
		} break;
	}
	if (submenu->type == menu_type_internal) {
			assert (submenu == &submenu_delete_confirmation);
			auto dim = menu_ItemDimensions("Really delete this file?");
			delete_confirmation_text_offset.x = (self->dimensions.w - dim.w) / 2;
			delete_confirmation_text_offset.y = resources_framework_font.line_height * 3;
			self->dimensions.y -= delete_confirmation_text_offset.y;
	}
	self->dimensions.topoffset = top_item_offset;
	if (submenu->show_name) {
		self->dimensions.y -= MENU_NAME_VERTICAL_DISTANCE_ABOVE / 2;
		if (self->dimensions.y + self->dimensions.h + MENU_NAME_DISTANCE_BELOW_TOP > RESOLUTION_HEIGHT)
			self->dimensions.y = RESOLUTION_HEIGHT - MENU_NAME_DISTANCE_BELOW_TOP - self->dimensions.h;
		if (self->dimensions.y < MENU_DISTANCE_ABOVE_BOTTOM) {
			int h = self->dimensions.h + MENU_NAME_VERTICAL_DISTANCE_ABOVE;
			if (self->dimensions.h > RESOLUTION_HEIGHT - MENU_DISTANCE_ABOVE_BOTTOM - MENU_NAME_DISTANCE_BELOW_TOP) {
				if (self->dimensions.h > RESOLUTION_HEIGHT) {
					assert (false);
					self->dimensions.y = 0;
				}
				else {
					self->dimensions.y = 0;
				}
			}
			else {
				self->dimensions.y = MENU_DISTANCE_ABOVE_BOTTOM + (RESOLUTION_HEIGHT - MENU_NAME_DISTANCE_BELOW_TOP - MENU_DISTANCE_ABOVE_BOTTOM - self->dimensions.h) / 2;
			}
		}
	}
}

static void SwitchMenu (menu_t *self, submenu_t *new_menu) {
	auto submenu = self->submenu;
	int was_selected = submenu->selected;
    if (!submenu->retain_selection) submenu->selected = 0;
	if (new_menu == NULL) new_menu = submenu->parent;
	else { new_menu->parent = submenu; }
	if (new_menu == submenu->parent) {
		if (submenu->on_exit_func) submenu->on_exit_func ();
	}
	assert (new_menu != NULL);
	auto oldmenu = self->submenu;
	if (new_menu == &submenu_delete_confirmation) {
		assert (new_menu == &submenu_delete_confirmation);
		delete_confirmation_explorer_file_index = was_selected;
		delete_confirmation_filename = menu_explorer.filenames[delete_confirmation_explorer_file_index];
		self->submenu = &submenu_delete_confirmation;
		submenu = self->submenu;
		submenu->parent = oldmenu;
		delete_confirmed = false;
		submenu->selected = 1; // Default to the cancel option rather than delete to prevent accidental deletions
	}
	else {
		self->submenu = new_menu;
		submenu = self->submenu;
		submenu_Init (self->submenu);
	}
	menu_CalculateDimensions (self);
}

static void SelectItem (menu_t *self, int item_index) {
	__label__ goto_select_item_list;
	if (self->onConfirm) self->onConfirm ();
    auto submenu = self->submenu;
    if (item_index < 0) return;
    switch (submenu->type) {
        case menu_type_list: {
		goto_select_item_list:
            if (item_index > submenu->list.item_count) return;
            auto item = submenu->list.items[item_index];
            switch (item.type) {
				case menu_list_item_type_slider: {
					item.slider.Function (item.slider.value_0_to_255);
				} break;
                case menu_list_item_type_function: {
                    item.Function ();
                } break;
                case menu_list_item_type_submenu: {
                    // menu_Construct (item.submenu);
                    SwitchMenu (self, item.submenu);
                } break;
				case menu_list_item_type_toggle: {
					*item.toggle.var = !*item.toggle.var;
					if (item.toggle.on_toggle_func) item.toggle.on_toggle_func (*item.toggle.var);
				} break;
            }
        } break;

        case menu_type_explorer: {
            submenu->selected = item_index;
            if (item_index > menu_explorer.file_count) return;
            if ( (!menu_explorer.is_folder[item_index] && (submenu->explorer.selectable & MENU_EXPLORER_SELECTABLE_FILES))
                || (menu_explorer.is_folder[item_index] && (submenu->explorer.selectable & MENU_EXPLORER_SELECTABLE_FOLDERS) && menu_explorer.depth == submenu->explorer.maximum_depth))
                submenu->explorer.selection_func (menu_explorer.current_directory_string, menu_explorer.filenames[submenu->selected]);
            else if (menu_explorer.is_folder[item_index] && menu_explorer.depth < submenu->explorer.maximum_depth) {
                explorer_OpenSubDirectory (&menu_explorer, menu_explorer.filenames[item_index]);
                menu_CalculateDimensions (self);
            }
        } break;

		case menu_type_name_creator: {
			submenu->name_creator.confirm_func ();
		} break;

		case menu_type_internal: {
			assert (submenu == &submenu_delete_confirmation);
			goto goto_select_item_list;
		} break;
    }
};

void MenuBack (menu_t *self) {
	auto submenu = self->submenu;
	if (self->onConfirm) self->onConfirm ();
    if (submenu->type == menu_type_explorer && menu_explorer.depth > 0) {
        char *source_child = menu_explorer.current_directory_string;
        char *c = source_child + strlen(source_child) - 2;
        while (*c != '/' && c > source_child) --c;
        ++c;
        char child_name[512];
        sprintf (child_name, c);
        explorer_GoUpOneDirectory (&menu_explorer);
        for (int i = 0; i < menu_explorer.file_count; ++i) {
            if (StringsAreTheSameCaseInsensitive (menu_explorer.filenames[i], child_name)) {
                submenu->selected = i;
                break;
            }
        }
        menu_CalculateDimensions (self);
    }
    else if (submenu->parent != NULL) {
        const char *source_child = submenu->name;
        SwitchMenu (self, NULL);
    }
};

static int ItemAt (menu_t *self, int x, int y, menu_inputs_t inputs) {
	__label__ goto_item_at_list;
    auto submenu = self->submenu;
    x -= self->dimensions.x;
    y -= self->dimensions.y;
    if (x < -XMARGIN || x > self->dimensions.w-1+XMARGIN || y < 0 || y > self->dimensions.h-1) return -1;
    int item_count = 0;
	int addon = 0;
    switch (submenu->type) {
        case menu_type_list: {
		goto_item_at_list:
			item_count = submenu->list.item_count; 
		} break;
        case menu_type_explorer: {
            int file_count = menu_explorer.file_count;
            int explorer_start = 0;
            int explorer_end = file_count;
            if (file_count > 17) {
                explorer_start = submenu->selected - 8;
                if (explorer_start < 0) explorer_start = 0;
                explorer_end = explorer_start + 17;
                if (explorer_end > file_count) {
                    explorer_end = file_count;
                    explorer_start = explorer_end - 17;
                }
            }
            item_count = explorer_end - explorer_start;
			addon = explorer_scroll;
        } break;
		case menu_type_name_creator: return 0;

		case menu_type_internal: {
			assert (submenu == &submenu_delete_confirmation);
			goto goto_item_at_list;
		} break;
    }
    int divisor = self->dimensions.h / item_count;
    return item_count-1 - (inputs.mouse.y - self->dimensions.y) / divisor + addon;
};

static bool menu_NameCreatorDeleteCharacter (typeof((submenu_t){}.name_creator) *self) {
	size_t len = strlen (self->text_buffer);
	if (self->cursor >= len) return false;
	for (int i = self->cursor; i < len-1; ++i)
		self->text_buffer[i] = self->text_buffer[i+1];
	self->text_buffer[len-1] = 0;
	return true;
}

void menu_Update (menu_t *self, menu_inputs_t input) {
	__label__ goto_update_list;
    static menu_inputs_t input_prev = {}; 
    auto submenu = self->submenu;
	mouse.x = input.mouse.x;
	mouse.y = input.mouse.y;
	static int mouse_dragging = -1;
	int mouse_over_item = ItemAt (self, mouse.x, mouse.y, input);
	if (!(input.mouse.left & (KEY_PRESSED | KEY_HELD))) mouse_dragging = -1;
	bool left_click = input.mouse.left & KEY_PRESSED;

	switch (submenu->type) {
		case menu_type_list: {
		goto_update_list:
			auto item = &submenu->list.items[submenu->selected];
			if (input.up)
				if (--submenu->selected < 0)
					submenu->selected = submenu->list.item_count-1;
			if (input.down)
				if (++submenu->selected >= submenu->list.item_count)
					submenu->selected = 0;
			switch (item->type) {
				case menu_list_item_type_toggle:
				case menu_list_item_type_submenu:
				case menu_list_item_type_function: break;

				case menu_list_item_type_slider: {
					auto before = item->slider.value_0_to_255;
					if (input.left && item->slider.value_0_to_255 > 0) --item->slider.value_0_to_255;
					if (input.right && item->slider.value_0_to_255 < item->slider.max) ++item->slider.value_0_to_255;
					int l = self->dimensions.x + self->dimensions.textw + SLIDER_MARGIN + 1;
					int r = l + SLIDER_WIDTH - 2; // -1 for slider border, -1 for normal width off-by-one calculations
					int mx = mouse.x + 1;
					if ((mouse_dragging == -1 && mouse_over_item == submenu->selected && input.mouse.left & KEY_PRESSED && mx >= l-3 && mx <= r+3)
					|| (mouse_dragging == submenu->selected)) {
						if (mx < l) mx = l;
						if (mx > r) mx = r;
						item->slider.value_0_to_255 = (float)(mx - l) / (SLIDER_WIDTH - 2) * item->slider.max;
						mouse_dragging = submenu->selected;
					}
					if (item->slider.value_0_to_255 != before)
						item->slider.Function (item->slider.value_0_to_255);
				} break;
			}
			if (input.confirm) {
				SelectItem (self, submenu->selected);
			}
		} break;

		case menu_type_explorer: {
			bool updown = false;
			if (input.up) {
				updown = true;
				if (submenu->selected == 0)
					submenu->selected = menu_explorer.file_count-1;
				else --submenu->selected;
			}
			if (input.down) {
				updown = true;
				if (++submenu->selected >= menu_explorer.file_count)
					submenu->selected = 0;
			}
			if (updown) {
				if (explorer_scroll < submenu->selected - EXPLORER_ITEMS_TO_SHOW + 1) {
					explorer_scroll = submenu->selected - EXPLORER_ITEMS_TO_SHOW + 1;
				}
				if (submenu->selected < explorer_scroll) {
					explorer_scroll = submenu->selected;
				}
			}
			if (input.mouse.scroll != 0) {
				if (input.mouse.scroll > 0 && explorer_scroll > 0) {
					--explorer_scroll;
					if (submenu->selected > explorer_scroll + EXPLORER_ITEMS_TO_SHOW - 1) submenu->selected = explorer_scroll + EXPLORER_ITEMS_TO_SHOW - 1;
				}
				else if (input.mouse.scroll < 0 && explorer_scroll < menu_explorer.file_count - EXPLORER_ITEMS_TO_SHOW) {
					++explorer_scroll;
					if (submenu->selected < explorer_scroll) submenu->selected = explorer_scroll;
				}
			}
			if (input.confirm) {
				SelectItem (self, submenu->selected);
			}
			if (left_click) {
				if (input.mouse.x >= title_bounds.l && input.mouse.x <= title_bounds.r && input.mouse.y >= title_bounds.b && input.mouse.y <= title_bounds.t) {
					os_OpenFileBrowser (menu_explorer.current_directory_string);
				}
			}
			if (input.delete) {
				// Confirm menu
				SwitchMenu (self, &submenu_delete_confirmation);
			}
		} break;

		case menu_type_name_creator: {
			const auto fc = &submenu->name_creator;
			if (input.left && fc->cursor > 0) --fc->cursor;
			if (input.right && fc->cursor < strlen (fc->text_buffer)) ++fc->cursor;
			if (input.backspace && fc->cursor > 0) {
				--fc->cursor;
				menu_NameCreatorDeleteCharacter(fc);
			}
			if (input.delete)
				menu_NameCreatorDeleteCharacter(fc);
			constexpr bool character_illegal[256] = {['/']=1,['\\']=1, [':']=1,['*']=1,['?']=1,['"']=1,['<']=1,['>']=1,['|']=1,};
			for (int i = 0; i < sizeof (input.typing) && input.typing[i] != 0; ++i) {
				char c = input.typing[i];
				if (character_illegal[(int)c]) continue;
				if (c < ' ' || c > '~') continue;
				size_t len = strlen (fc->text_buffer);
				if (len < *fc->buffer_size-1) {
					for (int j = len; j > fc->cursor; --j)
						fc->text_buffer[j] = fc->text_buffer[j-1];
					fc->text_buffer[fc->cursor] = c;
					++fc->cursor;
				}
			}
			if (input.confirm)
				fc->confirm_func ();
			menu_CalculateDimensions (self);
		} break;

		case menu_type_internal: {
			assert (submenu == &submenu_delete_confirmation);
			if (delete_confirmed) {
				if (explorer_DeleteFile (&menu_explorer, delete_confirmation_explorer_file_index)) explorer_ReloadDirectory (&menu_explorer);
				MenuBack (self);
			}
			else goto goto_update_list;
		} break;
	}
	
	if (input.cancel) {
		MenuBack (self);
	}

	// LOG ("Mouse: %d,%d\t%d->%d, %d->%d", mouse.x, mouse.y, self->dimensions.x, self->dimensions.x + self->dimensions.w, self->dimensions.y, self->dimensions.y + self->dimensions.h);
	if (input.mouse.x != input_prev.mouse.x || input.mouse.y != input_prev.mouse.y) {
		if (input.mouse.x >= self->dimensions.x && input.mouse.x < self->dimensions.x + self->dimensions.w
		 && input.mouse.y >= self->dimensions.y && input.mouse.y < self->dimensions.y + self->dimensions.h) {
			submenu->selected = ItemAt (self, input.mouse.x, input.mouse.y, input);
		}
	}

	if (left_click) {
		if (input.mouse.x >= self->dimensions.x - XMARGIN && input.mouse.x < self->dimensions.x + self->dimensions.w - 1 + XMARGIN
		&& input.mouse.y >= self->dimensions.y && input.mouse.y < self->dimensions.y + self->dimensions.h) SelectItem (self, ItemAt (self, input.mouse.x, input.mouse.y, input));
		else if (submenu->parent != NULL && input.mouse.x >= 0 && input.mouse.x <= resources_framework_menu_back.w && input.mouse.y >= RESOLUTION_HEIGHT - resources_framework_menu_back.h && input.mouse.y <= RESOLUTION_HEIGHT) MenuBack (self);
	}

    input_prev = input;
}

void menu_Render (menu_t *self, int depth) {
	__label__ goto_render_list, goto_draw_box_list;
    auto submenu = self->submenu;
    switch (self->background) {
        case background_type_none: break;
        case background_type_checkers: {
            static int32split_t bgx = {.i32 = 0}, bgy = {.i32 = 0};
            Render_Background(.type = background_type_checkers, .checkers.color = submenu->color, .checkers.width = 120, .checkers.height = 120, .checkers.x = bgx.high, .checkers.y = bgy.high);
            bgx.i32 += UINT16_MAX / 3;
            bgy.i32 += UINT16_MAX / 4;
        } break;
        default: break;
    }

    if (self->transparent_background_darkess) {
        const int16_t l = self->dimensions.x - 2;
        const int16_t r = l + self->dimensions.w-1 + 3;
        const int16_t b = self->dimensions.y;
        const int16_t t = b + self->dimensions.h-1;
        Render_DarkenRectangle (.l = l, .r = r, .b = b, .t = t, .depth = depth, .levels = self->transparent_background_darkess);
    }

	int explorer_start = 0;
	int explorer_end = 0;

	int y = self->dimensions.y + self->dimensions.h - 1 + self->dimensions.topoffset;
	int x = self->dimensions.x;

	if (submenu->show_name) {
		int yy = y + MENU_NAME_VERTICAL_DISTANCE_ABOVE;
		switch (submenu->type) {
			case menu_type_name_creator: yy += 30; break;
			case menu_type_explorer: break;
			case menu_type_list: break;
			case menu_type_internal: break;
		}
		Render_Text (.x = x, .y = yy, .string = submenu->name, .depth = depth, .ignore_camera = true, .center_horizontally_on_screen = true);
	}

	switch (submenu->type) {
		case menu_type_internal: {
			goto goto_draw_box_list;
		} break;
		case menu_type_name_creator: break;
		case menu_type_explorer:
		case menu_type_list: {
		goto_draw_box_list:
			render_shape_t rectangle = {
				.type = render_shape_rectangle,
				.rectangle = {
					.w = self->dimensions.w-1 + XMARGIN * 2,
					.h = resources_framework_font.line_height,
					.x = self->dimensions.x - XMARGIN,
					.color_edge = self->selection_color,
				}
			};
			rectangle.rectangle.y = self->dimensions.y + self->dimensions.h - (submenu->selected - explorer_start) * resources_framework_font.line_height - rectangle.rectangle.h;
			Render_Shape (.shape = rectangle, .depth = depth, .ignore_camera = true);
		} break;
	}

	switch (submenu->type) {
		case menu_type_list: {
		goto_render_list:
			auto item = submenu->list.items;
			repeat (submenu->list.item_count) {
				Render_Text (.x = x, .y = y, .string = item->name, .depth = depth, .ignore_camera = true);
				switch (item->type) {
					case menu_list_item_type_function:
					case menu_list_item_type_submenu:
						break;
					case menu_list_item_type_slider: {
						int b = y - resources_framework_font.line_height/2 - 3;
						int l = x + self->dimensions.textw + SLIDER_MARGIN;
						Render_Shape (.shape = {.type = render_shape_rectangle, .rectangle = {.color_edge = self->selection_color, .x = l, .w = SLIDER_WIDTH, .y = b, .h = 3}}, .depth = depth, .ignore_camera = true);
						int w = (SLIDER_WIDTH - 3) * (item->slider.value_0_to_255 / (float)item->slider.max);
						if (item->slider.value_0_to_255 > 0) Render_Shape (.shape = {.type = render_shape_line, .line = {.color = 255, .x0 = l+1, .x1 = l+1+w, .y0 = b+1, .y1 = b+1}}, .depth = depth, .ignore_camera = true);
					} break;
					case menu_list_item_type_toggle: {
						int b = y - resources_framework_font.line_height;
						int l = x + self->dimensions.textw + SLIDER_MARGIN;
						uint8_t fill_color = 0;
						if (*item->toggle.var) fill_color = 255;
						Render_Shape (.shape = {.type = render_shape_rectangle, .rectangle = {.color_fill = fill_color, .color_edge = self->selection_color, .x = l, .w = TOGGLE_WIDTH, .y = b, .h = TOGGLE_WIDTH}}, .depth = depth, .ignore_camera = true);
					} break;
				}
				y -= resources_framework_font.line_height;
				++item;
			}
		} break;

		case menu_type_explorer: {
			int file_count = menu_explorer.file_count;
			explorer_start = explorer_scroll;
			explorer_end = explorer_scroll + EXPLORER_ITEMS_TO_SHOW;
			if (explorer_end > file_count) explorer_end = file_count;
			Render_Text (.x = x, .y = y + 20, .string = submenu->explorer.show_name_instead_of_directory ? submenu->name : menu_explorer.current_directory_string, .depth = depth, .ignore_camera = true);
			for (int i = explorer_start; i < explorer_end; ++i) {
				if (i == submenu->selected)
					Render_Text (.x = x-10, .y = y, .string = ">", .depth = depth, .ignore_camera = true);
				Render_Text (.x = x, .y = y, .string = menu_explorer.filenames[i], .depth = depth, .ignore_camera = true);
				y -= resources_framework_font.line_height;
			}
			Render_Sprite (.sprite = &resources_framework_menu_folder_open, .x = RESOLUTION_WIDTH - resources_framework_menu_folder_open.w, .y = RESOLUTION_HEIGHT - resources_framework_menu_folder_open.h, .depth = depth, .ignore_camera = true);
		} break;

		case menu_type_name_creator: {
			const auto fc = &submenu->name_creator;
			y = RESOLUTION_HEIGHT / 2;
			int texty = y + resources_framework_font.baseline;
			Render_Shape (.shape = {.type = render_shape_rectangle, .rectangle = {.x = x, .y = y - 1, .w = self->dimensions.w, .h = 1, .color_edge = self->selection_color}}, .depth = depth);
			auto cursorx = menu_ItemDimensions_Length(fc->text_buffer, fc->cursor).x + x;
			vec2i_t cursorsize;
			if (fc->cursor == strlen (fc->text_buffer)) {
				cursorsize.x = 1;
				cursorsize.y = resources_framework_font.baseline;
			}
			else {
				cursorsize = menu_ItemDimensions_Length(&fc->text_buffer[fc->cursor], 1);
			}
			Render_Shape (.shape = {.type = render_shape_rectangle, .rectangle = {.x = cursorx, .y = y, .w = cursorsize.x, .h = cursorsize.y, .color_edge = 62, .color_fill = 62}}, .depth = depth);
			Render_Text (.x = x, .y = y + resources_framework_font.line_height, .string = fc->text_buffer, .depth = depth, .ignore_camera = true);
		} break;
		case menu_type_internal: {
			Render_Text (.x = x + delete_confirmation_text_offset.x, .y = y + delete_confirmation_text_offset.y, .string = "Really delete this file?", .depth = depth, .ignore_camera = true);
			Render_Text (.x = x + delete_confirmation_text_offset.x, .y = y + delete_confirmation_text_offset.y - resources_framework_font.line_height, .string = delete_confirmation_filename, .depth = depth, .ignore_camera = true);
			goto goto_render_list;
		} break;
	}

	if (submenu->parent != NULL) Render_Sprite (.sprite = &resources_framework_menu_back, .y = RESOLUTION_HEIGHT - resources_framework_menu_back.h, .depth = depth, .ignore_camera = true);

    Render_CursorAtRawMousePos (&resources_framework_cursor);
}

static void menu_Delete_Yes () {
	delete_confirmed = true;
}