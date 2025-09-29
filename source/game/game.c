#include "game.h"

void game_Init ();
void game_Update () {}

const update_state_functions_t state_functions[update_state_count] = {
    [update_state_init] = {game_Init, game_Update}
};

extern update_data_t update_data;

void game_Init () {
    abort ();
}