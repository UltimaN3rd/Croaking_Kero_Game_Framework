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