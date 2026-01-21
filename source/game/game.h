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

#include "framework.h"
#include "resources.h"
#include "objects/text_popup.h"

typedef struct {
	uint8_t music_volume, fx_volume;
	bool fullscreen;
	struct {
		bool show_framerate, show_simtime, show_rendertime;
	} debug;
} submenu_vars_t;
extern submenu_vars_t submenu_vars;

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
void SaveGame ();
