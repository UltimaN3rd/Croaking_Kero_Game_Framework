#pragma once

#define GAME_TITLE "Flappy Choppa"

#define RESOLUTION_WIDTH 320
#define RESOLUTION_HEIGHT 240

typedef enum { update_state_init, update_state_gameplay, update_state_menu } update_state_e;
#define update_state_count update_state_menu+1

extern char GAME_FOLDER_SAVES[];
extern char GAME_FOLDER_CONFIG[];

#include "framework.h"

extern const update_state_functions_t state_functions[update_state_count];

typedef struct {
	uint8_t music_volume, fx_volume;
	bool fullscreen;
	struct {
		bool show_framerate, show_simtime, show_rendertime;
	} debug;
} submenu_vars_t;
extern submenu_vars_t submenu_vars;

void menu_Options_Fullscreen ();
void menu_Options_Sound_Music (float slider_0_to_1);
float menu_Options_Sound_Music_Init ();
void menu_Options_Sound_Effects (float slider_0_to_1);
float menu_Options_Sound_Effects_Init ();
void menu_Options_OnExit ();
void menu_Options_Debug_OpenFolder ();

extern submenu_t submenus_options[];
extern const cereal_t cereal[];

typedef struct {
	struct {
		char name[17];
		uint64_t score;
	} high_score;
} game_save_data_t;

extern game_save_data_t game_save_data;
void game_SaveGame ();