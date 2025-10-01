#include "game.h"

#define GRAVITY (-2048)
#define PLAYERX 80
#define PLAYER_FLAP_SPEED (-GRAVITY * 40)
#define PIPES_MAX 8
#define PIPE_GAP_MIN 70
#define PIPE_GAP_MAX 120
#define PIPE_SPEED 65536
#define PIPE_MAX_HEIGHT_DIFFERENCE_FROM_LAST 80
#define PLAYER_COIN_LIMIT UINT64_MAX
#define PLAYER_FLAP_PROPELLER_SPEED (2<<20)

typedef enum {state_alive, state_dead} gameplay_state_e;
#define GAMEPLAY_STATE_COUNT (state_dead+1)

static struct {
    struct {
        int32split_t y;
        int32_t vy;
        float pitch;
        uint64_t coins;
        int32_t propeller_speed;
        int32split_t propeller;
    } player;
    struct {
        int32split_t x;
        int8_t bottom;
        int8_t gap_size;
        bool has_been_passed;
    } pipes[PIPES_MAX];
    gameplay_state_e state;
    struct {
        uint8_t cooldown;
        bool new_high_score;
    } dead;
} data;

static uint64_t random_state;
extern update_data_t update_data;

const sound_t sound_coin2 = {.sample = &sound_sample_sine, .duration = 48000 * .1, .frequency_begin = 2000, .frequency_end = 2000, .ADSR = {ADSR_DEFAULT}};
const sound_t sound_coin = {.sample = &sound_sample_sine, .duration = 48000 * .1, .next = &sound_coin2, .frequency_begin = 1600, .frequency_end = 1600, .ADSR = {ADSR_DEFAULT}};

void gameplay_Exit ();
void PipeGenerate (int i);
void PlayerGetCoin ();
void PlayerFlap ();
void PlayerDie ();

void UpdateAlive ();
void UpdateDead ();
void SaveHighScore ();

static submenu_t menu_high_score_name_entry = {
    "New high score",
    .type = menu_type_name_creator,
    .name_creator = {
        .text_buffer = (char[]){[sizeof((game_save_data_t){}.high_score.name)] = 0},
        .buffer_size = &(const size_t){sizeof((game_save_data_t){}.high_score.name)},
        .confirm_func = SaveHighScore,
    },
};
static menu_t menu_death = {.background = background_type_blank, .submenu = &menu_high_score_name_entry};

void (*StateUpdate[GAMEPLAY_STATE_COUNT]) () = {
    [state_alive] = UpdateAlive,
    [state_dead] = UpdateDead,
};

void gameplay_Init () {
    data = (typeof(data)){};
    data.player.y.high = 200;

    random_state = time (0);

    data.pipes[0].x.high = RESOLUTION_WIDTH + resources_gameplay_pipe_top.w/2;
    data.pipes[0].bottom = RESOLUTION_HEIGHT / 2 - 20;
    data.pipes[0].gap_size = PIPE_GAP_MAX;
    for (int i = 1; i < PIPES_MAX; ++i) PipeGenerate (i);

    SoundMusicPlay (&resources_music_ost1);
}

void gameplay_Update () {
    auto keyboard = update_data.frame.keyboard_state;
    
    if (keyboard[os_KEY_ESCAPE] & KEY_PRESSED) {
        gameplay_Exit ();
        return;
    }

    Render_Background (.type = background_type_blank, .blank = {.color = 193});
    
    StateUpdate[data.state] ();

    int depth = -1;

    for (int i = 0; i < PIPES_MAX; ++i) {
        int x = data.pipes[i].x.high;
        int b = data.pipes[i].bottom;
        int t = b + data.pipes[i].gap_size;
        Render_Sprite (.sprite = &resources_gameplay_pipe_top, .x = x, .y = b, .sprite_flags.center_horizontally = true, .originy = resources_gameplay_pipe_top.h-1, .depth = -1);
        Render_Sprite (.sprite = &resources_gameplay_pipe_top, .x = x, .y = t, .originy = resources_gameplay_pipe_top.h-1, .sprite_flags = {.center_horizontally = true, .flip_vertically = true}, .depth = -1);
        for (int y = b - resources_gameplay_pipe_top.h; y >= 0; y -= resources_gameplay_pipe_body.h)
            Render_Sprite (.sprite = &resources_gameplay_pipe_body, .x = x, .y = y, .sprite_flags.center_horizontally = true, .originy = resources_gameplay_pipe_body.h-1, .depth = -1);
        for (int y = t + resources_gameplay_pipe_top.h; y < RESOLUTION_HEIGHT; y += resources_gameplay_pipe_body.h)
            Render_Sprite (.sprite = &resources_gameplay_pipe_body, .x = x, .y = y, .sprite_flags.center_horizontally = true, .depth = -1);
        if (x > PLAYERX) Render_Sprite (.sprite = &resources_gameplay_coin, .x = x, .y = (b + t) / 2, .sprite_flags = {.center_horizontally = true, .center_vertically = true}, .depth = -1);
    }

    {
        int x = 1;
        int y = RESOLUTION_HEIGHT - 2;
        Render_Sprite (.sprite = &resources_gameplay_coin, .x = x, .y = y, .originy = resources_gameplay_coin.h-1, .depth = -1);
        x += resources_gameplay_coin.w + 1;
        y += 1;
        char buf[32];
        snprintf (buf, sizeof(buf), "%"PRIu64, data.player.coins);
        Render_Text (.string = buf, .x = x, .y = y, .depth = 1);
    }
}

void gameplay_Exit () {
    SoundMusicStop ();
    Update_ChangeState (update_state_menu);
}

void PipeGenerate (int i) {
    assert (i > 0);
    assert (i < PIPES_MAX);
    data.pipes[i] = (typeof(data.pipes[0])){
        .x.high = data.pipes[i-1].x.high + DiscreteRandom_Range(&random_state, 80, 120),
        .bottom = MIN (RESOLUTION_HEIGHT-1 - resources_gameplay_pipe_top.h - PIPE_GAP_MAX, MAX (resources_gameplay_pipe_top.h, data.pipes[i-1].bottom + DiscreteRandom_Range(&random_state, -PIPE_MAX_HEIGHT_DIFFERENCE_FROM_LAST/2, PIPE_MAX_HEIGHT_DIFFERENCE_FROM_LAST/2))),
        .gap_size = DiscreteRandom_Range(&random_state, PIPE_GAP_MIN, PIPE_GAP_MAX),
    };
}

void PlayerGetCoin () {
    if (data.player.coins == PLAYER_COIN_LIMIT) {
        // Do something secret. Of course no player will ever hit this many coins by playing normally
        return;
    }
    ++data.player.coins;
    SoundFXPlayDirect (sound_coin);
}

void PlayerFlap () {
    data.player.propeller_speed = PLAYER_FLAP_PROPELLER_SPEED;
    data.player.vy = PLAYER_FLAP_SPEED;
}

void PlayerDie () {
    data.state = state_dead;
    data.dead = (typeof(data.dead)){
        .cooldown = 60,
    };
    CreateParticlesFromSprite(&resources_gameplay_heli, PLAYERX, data.player.y.high, .25f, 2<<16, .rotation = data.player.pitch, .originx = resources_gameplay_heli.w/2, .originy = resources_gameplay_heli.h/2);

    if (data.player.coins > game_save_data.high_score.score) {
        data.dead.new_high_score = true;
        memset(menu_high_score_name_entry.name_creator.text_buffer, 0, *menu_high_score_name_entry.name_creator.buffer_size);
        menu_high_score_name_entry.name_creator.cursor = 0;
    }
}


void UpdateAlive () {
    auto keyboard = update_data.frame.keyboard_state;

    data.player.vy += GRAVITY;
    if (keyboard[os_KEY_SPACE] & KEY_PRESSED) PlayerFlap ();

    data.player.y.i32 += data.player.vy;
    data.player.propeller_speed *= .95f;
    data.player.propeller.i32 += data.player.propeller_speed;
    data.player.pitch = (data.player.pitch * 0.9f) + (data.player.vy * 0.000002f * 0.1f);
    if (fabs (data.player.pitch) > 0.25) data.player.pitch = SIGN (data.player.pitch) * 0.25f;

    if (data.player.y.high > RESOLUTION_HEIGHT || data.player.y.high < 0) PlayerDie ();

    if (data.pipes[0].x.high < -resources_gameplay_pipe_top.w/2) {
        for (int j = 0; j < PIPES_MAX-1; ++j) {
            data.pipes[j] = data.pipes[j+1];
        }
        PipeGenerate (PIPES_MAX-1);
    }

    uint8_t nearest_pipe = 0;
    uint8_t pipe_distance = UINT8_MAX;

    for (int i = 0; i < PIPES_MAX; ++i) {
        data.pipes[i].x.i32 -= PIPE_SPEED;
        int16_t distance = abs (PLAYERX - data.pipes[i].x.high);
        if (distance < pipe_distance) {
            nearest_pipe = i;
            pipe_distance = distance;
        }
        if (data.pipes[i].x.high < PLAYERX && !data.pipes[i].has_been_passed) {
            data.pipes[i].has_been_passed = true;
            PlayerGetCoin ();
        }
    }

    if (pipe_distance < resources_gameplay_pipe_top.w/2 + 8) {
        if (data.player.y.high - 5 < data.pipes[nearest_pipe].bottom
         || data.player.y.high + 5 > data.pipes[nearest_pipe].bottom + data.pipes[nearest_pipe].gap_size)
            PlayerDie ();
    }

    Render_Sprite (.sprite = &resources_gameplay_heli, .x = PLAYERX, .y = data.player.y.high, .rotation = data.player.pitch, .sprite_flags = {.center_horizontally = true, .center_vertically = true});
    // Propellers
    {
        float spin = data.player.propeller.high / 360.0;
        repeat (2) {
            float xprop = sin_turns (spin);
            float yprop = sin_turns (spin + 0.25);
            int y = (resources_gameplay_heli.h+1)/2;
            float p = data.player.pitch - 0.0225f; // For some reason the propellers weren't lining up quite right - they always seemed to be a tiny bit further rotated around the player than they were supposed to be, and this fixed it, oddly. Not sure What's wrong with my math since it's working fine in Heli.
            float originx = -y * sin_turns (p) + 0.5f;
            float originy = y * cos_turns (p) + 0.5f;
            auto localpos = RotatePointiTurns(xprop * 12, yprop * 3, p);
            Render_Shape (.shape = {.type = render_shape_line, .line = {.color = 1, .x0 = PLAYERX + originx + localpos.x, .x1 = PLAYERX + originx - localpos.x, .y0 = data.player.y.high + originy + localpos.y, .y1 = data.player.y.high + originy - localpos.y}});
            spin += 0.25;
        }
    }
}

void UpdateDead () {
    auto keyboard = update_data.frame.keyboard_state;
	auto mouse = update_data.frame.mouse;
	auto mouse_buttons = update_data.frame.mouse_state;

    if (data.dead.cooldown > 0) {
        --data.dead.cooldown;
    }
    else {
        if (data.dead.new_high_score) {
            Render_Text (.string = "New high score!!!\nEnter your name:", .x = 110, .y = 200);
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
            menu_Update (&menu_death, inputs);
            menu_Render (&menu_death, 20);
        }
        else {
            if (keyboard[os_KEY_SPACE] & KEY_PRESSED) {
                gameplay_Init ();
            }
            Render_Text (.string = "Press [SPACE] to play again", .x = 90, .y = 140, .depth = 10);
        }
    }
}

void SaveHighScore () {
    snprintf (game_save_data.high_score.name, sizeof(game_save_data.high_score.name), "%s", menu_high_score_name_entry.name_creator.text_buffer);
    game_save_data.high_score.score = data.player.coins;
    game_SaveGame ();
    
    data.dead.new_high_score = false;
}