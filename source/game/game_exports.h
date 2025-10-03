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

#define GAME_TITLE "Flappy Choppa"

#define RESOLUTION_WIDTH 320
#define RESOLUTION_HEIGHT 240

typedef enum { update_state_init, update_state_gameplay, update_state_menu } update_state_e;
#define update_state_count update_state_menu+1

extern char GAME_FOLDER_SAVES[];
extern char GAME_FOLDER_CONFIG[];

#include "framework.h"

extern const update_state_functions_t state_functions[update_state_count];
extern const uint8_t palette[256][3];
extern const font_t framework_font;