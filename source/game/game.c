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

void game_Init (const void *const ignore);
void game_Update () {}
void game_menu_Init (const void *const ignore);
void game_menu_Update ();
void gameplay_Init (const void *const ignore);
void gameplay_Update ();

const update_state_functions_t state_functions[update_state_count] = {
    [update_state_init] = {game_Init, game_Update},
    [update_state_menu] = {game_menu_Init, game_menu_Update},
    [update_state_gameplay] = {gameplay_Init, gameplay_Update},
};

extern update_data_t update_data;
extern const cereal_t cereal_options[];
extern const size_t cereal_options_size;

void game_Init () {
	{
		char buf[1024];
		snprintf (buf, sizeof (buf), "%s/%s", os_public.directories.savegame, GAME_FOLDER_SAVES);
		folder_CreateDirectoryRecursive(buf);
	}
	snprintf (update_data.config_filename, sizeof (update_data.config_filename), "%s/%s", os_public.directories.config, GAME_FOLDER_CONFIG);
	folder_CreateDirectoryRecursive(update_data.config_filename);
	snprintf (update_data.config_filename, sizeof (update_data.config_filename), "%s/%s/config", os_public.directories.config, GAME_FOLDER_CONFIG);
	cereal_ReadFromFile(cereal_options, cereal_options_size, update_data.config_filename);
	SoundMusicSetVolume (submenu_vars.music_volume / 255.f);
	SoundFXSetVolume (submenu_vars.fx_volume / 255.f);
	os_Fullscreen(submenu_vars.fullscreen);

	snprintf (update_data.game_save_filename, sizeof(update_data.game_save_filename), "%s/%s", os_public.directories.savegame, GAME_FOLDER_SAVES);
	folder_CreateDirectoryRecursive(update_data.game_save_filename);
	snprintf (update_data.game_save_filename, sizeof(update_data.game_save_filename), "%s/%s/high_score", os_public.directories.savegame, GAME_FOLDER_SAVES);
	cereal_ReadFromFile(cereal_savedata, cereal_savedata_size, update_data.game_save_filename);

	update_data.debug.show_framerate = &submenu_vars.debug.show_framerate;
	update_data.debug.show_rendertime = &submenu_vars.debug.show_rendertime;
	update_data.debug.show_simtime = &submenu_vars.debug.show_simtime;

	Update_ChangeState (update_state_menu);
}

#ifdef WIN32
char GAME_FOLDER_SAVES[] = "CroakingKero\\" GAME_TITLE;
char GAME_FOLDER_CONFIG[] = "CroakingKero\\" GAME_TITLE;
#elifdef __linux__
char GAME_FOLDER_SAVES[] = "CroakingKero/" GAME_TITLE;
char GAME_FOLDER_CONFIG[] = "CroakingKero/" GAME_TITLE;
#elifdef __APPLE__
char GAME_FOLDER_SAVES[] = "com.CroakingKero." GAME_TITLE;
char GAME_FOLDER_CONFIG[] = "com.CroakingKero." GAME_TITLE;
#else
#error "Unknown platform"
#endif

#define MENU_SLIDER_VOLUME_MAX 32

void menu_Options_Fullscreen () {
	os_Fullscreen (!os_public.window.is_fullscreen);
	submenu_vars.fullscreen = os_public.window.is_fullscreen;
}
void menu_Options_Sound_Music (uint8_t output_0_to_max) {
	SoundMusicSetVolume(output_0_to_max / (float)MENU_SLIDER_VOLUME_MAX);
	submenu_vars.music_volume = output_0_to_max;
}
uint8_t menu_Options_Sound_Music_Init () { return submenu_vars.music_volume; }
void menu_Options_Sound_Effects (uint8_t output_0_to_max) {
	SoundFXSetVolume(output_0_to_max / (float)MENU_SLIDER_VOLUME_MAX);
	submenu_vars.fx_volume = output_0_to_max;
}
uint8_t menu_Options_Sound_Effects_Init () { return submenu_vars.fx_volume; }

submenu_vars_t submenu_vars = {
	.music_volume = MENU_SLIDER_VOLUME_MAX * 0.6,
	.fx_volume = MENU_SLIDER_VOLUME_MAX,
	.fullscreen = false,
};
const cereal_t cereal_options[] = {
	{"Music volume", cereal_u8, &submenu_vars.music_volume},
	{"FX volume", cereal_u8, &submenu_vars.fx_volume},
	{"Fullscreen", cereal_bool, &submenu_vars.fullscreen},
	{"Debug show FPS", cereal_bool, &submenu_vars.debug.show_framerate},
	{"Debug show simulation time", cereal_bool, &submenu_vars.debug.show_simtime},
	{"Debug show render time", cereal_bool, &submenu_vars.debug.show_rendertime},
};
const size_t cereal_options_size = sizeof (cereal_options) / sizeof (*cereal_options);

#include "DEFER.h"
extern update_data_t update_data;
void menu_Options_OnExit () {
	cereal_WriteToFile(cereal_options, cereal_options_size, update_data.config_filename);
}

void menu_Options_Debug_OpenFolder () {
	char buf[1024];
	snprintf (buf, sizeof(buf), "%s/%s", os_public.directories.config, GAME_FOLDER_CONFIG);
	os_OpenFileBrowser(buf);
}

void menu_Options_Debug_OpenSaveFolder () {
	char buf[1024];
	snprintf (buf, sizeof(buf), "%s/%s", os_public.directories.savegame, GAME_FOLDER_SAVES);
	os_OpenFileBrowser(buf);
}

submenu_t submenus_options[] = {
	[0] = {
		.name = "Options",
		.color = 212,
		.type = menu_type_list,
		.retain_selection = true,
		.on_exit_func = menu_Options_OnExit,
		.list = {
			.item_count = 4,
			.items = {
				{.name = "Fullscreen", .type = menu_list_item_type_function, .Function = menu_Options_Fullscreen},
				{.name = "Sound", .type = menu_list_item_type_submenu, .submenu = &submenus_options[1]},
				{.name = "Debug", .type = menu_list_item_type_submenu, .submenu = &submenus_options[2]},
				{.name = "Back", .type = menu_list_item_type_submenu, .submenu = NULL},
			},
		},
	},
	[1] = {
		.name = "Sound",
		.color = 123,
		.type = menu_type_list,
		.retain_selection = true,
		.list = {
			.item_count = 3,
			.items = {
				{.name = "Music", .type = menu_list_item_type_slider, .slider = {.Function = menu_Options_Sound_Music, .InitialValueFunction = menu_Options_Sound_Music_Init, .max = MENU_SLIDER_VOLUME_MAX}},
				{.name = "Effects", .type = menu_list_item_type_slider, .slider = {.Function = menu_Options_Sound_Effects, .InitialValueFunction = menu_Options_Sound_Effects_Init, .max = MENU_SLIDER_VOLUME_MAX}},
				{.name = "Back", .type = menu_list_item_type_submenu, .submenu = NULL},
			},
		},
	},
	[2] = {
		.name = "Debug",
		.color = 160,
		.type = menu_type_list,
		.retain_selection = true,
		.list = {
			.item_count = 6,
			.items = {
				{.name = "Framerate", .type = menu_list_item_type_toggle, .toggle.var = &submenu_vars.debug.show_framerate},
				{.name = "Simulation time", .type = menu_list_item_type_toggle, .toggle.var = &submenu_vars.debug.show_simtime},
				{.name = "Rendering time", .type = menu_list_item_type_toggle, .toggle.var = &submenu_vars.debug.show_rendertime},
				{.name = "Open config/log folder", .type = menu_list_item_type_function, .Function = menu_Options_Debug_OpenFolder},
				{.name = "Open save data folder", .type = menu_list_item_type_function, .Function = menu_Options_Debug_OpenSaveFolder},
				{.name = "Back", .type = menu_list_item_type_submenu, .submenu = NULL},
			},
		},
	},
};

game_save_data_t game_save_data = {};

const cereal_t cereal_savedata[] = {
	{"Name", cereal_string, game_save_data.high_score.name, .u.string = {.capacity = sizeof (game_save_data.high_score.name)}},
	{"Score", cereal_u64, &game_save_data.high_score.score},
};

const size_t cereal_savedata_size = sizeof(cereal_savedata) / sizeof(*cereal_savedata);

void game_ToggleFullscreen () {
	os_Fullscreen (!os_public.window.is_fullscreen);
	submenu_vars.fullscreen = os_public.window.is_fullscreen;
	SaveSettings ();
}

void SaveGame () {
	if (!cereal_WriteToFile(cereal_savedata, cereal_savedata_size, game_save_file)) LOG ("Failed to save game");
}