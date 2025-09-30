#include "game.h"

#include <stdarg.h>

extern update_data_t update_data;
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

menu_t menu = {
	.background = background_type_checkers,
	.submenu_count = 2,
};

void menu_Function_Quit () {
	quit = true;
}

void game_menu_Init () {
	// auto keyboard = update_data.frame.keyboard_state;

	menu.submenu = &submenus[0];
	menu_CalculateDimensions (&menu);
	// SoundPlayMusic (&resources_music_ost1);
}

// static uint64_t random_state = 57490372;

void game_menu_Update () {
	auto keyboard = update_data.frame.keyboard_state;
	auto mouse = update_data.frame.mouse;
	auto mouse_buttons = update_data.frame.mouse_state;
	static vec2i_t last_mouse_position = {};
	auto m = menu.submenu;
	
	#define PRESSORREPEAT (KEY_PRESSED | KEY_REPEATED)
	menu_inputs_t inputs = {
		.up = keyboard[os_KEY_UP] & PRESSORREPEAT, .down = keyboard[os_KEY_DOWN] & PRESSORREPEAT, .left = keyboard[os_KEY_LEFT] & PRESSORREPEAT, .right = keyboard[os_KEY_RIGHT] & PRESSORREPEAT, .confirm = keyboard[os_KEY_ENTER] & KEY_PRESSED, .cancel = keyboard[os_KEY_ESCAPE] & KEY_PRESSED,
		.backspace = keyboard[os_KEY_BACKSPACE] & PRESSORREPEAT, .delete = keyboard[os_KEY_DELETE] & PRESSORREPEAT,
		.mouse = {.x = mouse.x, .y = mouse.y, .left = mouse_buttons[MOUSE_LEFT]}};
	int keys = 0;
	for (int i = os_KEY_FIRST_WRITABLE; i <= os_KEY_LAST_WRITABLE; ++i) {
		if (!(keyboard[i] & PRESSORREPEAT)) continue;
		char c;
		if (keyboard[os_KEY_SHIFT] & KEY_HELD) c = os_key_shifted (i);
		else c = i;
		inputs.typing[keys] = c;
		++keys;
		if (keys > sizeof (inputs.typing)) break;
	}
	menu_Update (&menu, inputs);

	menu_Render (&menu, 1);
} // menu ()

void menu_Function_NewGame () {
    Update_ChangeState (update_state_gameplay);
}