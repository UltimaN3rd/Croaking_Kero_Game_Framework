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

#include "game.h"
#include "resources.h"

#include <stdarg.h>

extern bool quit;

void menu_Function_NewGame ();
void menu_Function_Quit ();

static char new_game_filename_buffer[17];
static const size_t new_game_filename_buffer_size = sizeof (new_game_filename_buffer);

submenu_t submenus[] = {
	[0] = {
		.name = "Base",
		.color = 180,
		.type = menu_type_list,
		.retain_selection = true,
		.list.item_count = 3,
		.list.items = {
			{.name = "Play", .type = menu_list_item_type_function, .Function = menu_Function_NewGame},
			{.name = "Options", .type = menu_list_item_type_submenu, .submenu = &submenus_options[0]},
			{.name = "Quit", .type = menu_list_item_type_function, .Function = menu_Function_Quit},
		},
	},
};

menu_t menu = {.background = background_type_checkers};

void menu_Function_Quit () {
	quit = true;
}

void game_menu_Init () {
	menu_Init (&menu, &submenus[0]);
}

void game_menu_Update () {
	const auto input = *Update_FrameInput ();
	auto m = menu.submenu;
	
	#define PRESSORREPEAT (KEY_PRESSED | KEY_REPEATED)
	menu_inputs_t inputs = {
		.up = input.keyboard[os_KEY_UP] & PRESSORREPEAT, .down = input.keyboard[os_KEY_DOWN] & PRESSORREPEAT, .left = input.keyboard[os_KEY_LEFT] & PRESSORREPEAT, .right = input.keyboard[os_KEY_RIGHT] & PRESSORREPEAT, .confirm = input.keyboard[os_KEY_ENTER] & KEY_PRESSED, .cancel = input.keyboard[os_KEY_ESCAPE] & KEY_PRESSED,
		.backspace = input.keyboard[os_KEY_BACKSPACE] & PRESSORREPEAT, .delete = input.keyboard[os_KEY_DELETE] & PRESSORREPEAT,
		.mouse = {.x = input.mouse.x, .y = input.mouse.y, .left = input.mouse.buttons[MOUSE_LEFT]}};
	memcpy (inputs.typing, input.typing.chars, MIN (sizeof(inputs.typing), input.typing.count));
	menu_Update (&menu, inputs);

	menu_Render (&menu, 1);

	char buf[64];
	snprintf (buf, sizeof(buf), "High score:");
	Render_Text (.string = buf, .y = 224, .center_horizontally_on_screen = true);
	snprintf (buf, sizeof(buf), "%"PRIu64, game_save_data.high_score.score);
	Render_Text (.string = buf, .y = 210, .x = RESOLUTION_WIDTH/2);
	Render_Text (.string = game_save_data.high_score.name, .y = 196, .center_horizontally_on_screen = true);
	Render_Sprite (.sprite = &resources_gameplay_coin, .x = RESOLUTION_WIDTH/2-resources_gameplay_coin.w-1, .y = 197);
} // menu ()

void menu_Function_NewGame () {
    Update_ChangeState (update_state_gameplay);
}