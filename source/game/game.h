#pragma once

#include "framework.h"

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
extern const cereal_t cereal_savedata[];
extern const size_t cereal_savedata_size;