/*
    BSBOT made by William Dawson (MrBisquit on GitHub)
    https://wtdawson.info

    --------------------------------------------------------------------------------------------

    This is my battleship (the game) bot, entirely written in C.
    It was inspired by my other bot, nacbot.

    The ONLY dependencies are:
    *   memory.h
    *   malloc.h
    *   string.h
    *   stdio.h
    *   stdint.h
    *   stdlib.h
    *   time.h      This is used for the seed in the random algorithm
    *   raylib.h

    This uses Raylib, which is defined below.

    --------------------------------------------------------------------------------------------

 *  GitHub:     https://github.com/MrBisquit/bsbot
 *  File:       https://github.com/MrBisquit/bsbot/tree/master/src/main.c
 *  License:    SPDX-License-Identifier: MIT
 *              See LICENSE file in the project root for full license text or see below.

    --------------------------------------------------------------------------------------------

    MIT License

    Copyright (c) 2025 William Dawson

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#include <memory.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <raylib.h>

/*
    Below is the actual game, and the main functionality.
*/

/*
    Pre-definitions
*/

#define PLACE_BLANK 0
#define PLACE_AC    1 // Aircraft carrier
#define PLACE_BS    2 // Battleship
#define PLACE_DS    3 // Destroyer
#define PLACE_SB    4 // Submarine
#define PLACE_PB    5 // Patrol Boat
#define HIT_BLANK   0
#define HIT_AC      1 // Aircraft carrier
#define HIT_BS      2 // Battleship
#define HIT_DS      3 // Destroyer
#define HIT_SB      4 // Submarine
#define HIT_PB      5 // Patrol Boat

typedef struct {
    uint8_t a_places[10][10];
    uint8_t b_places[10][10];

    uint8_t a_hitmap[10][10];
    uint8_t b_hitmap[10][10];
} board_t;

typedef struct {
    float possibilities[10][10];
} bot_t;

/// @brief This is either an aircraft carrier, battleship, destroyer, submarine, or patrol boat
typedef struct {
    uint8_t type;
    uint8_t rotation; // 0 = Horizontal, 1 = Vertical
    uint8_t places; // How many places it takes up (horizontally or vertically)

    // These are defined for easily working things out during rendering
    Vector2 size_normal;
    Vector2 size_hovering;
} item_t;

typedef enum {
    GAME_STATE_MENU,
    GAME_STATE_SELECTION,
    GAME_STATE_DESTRUCTION
} game_state_t;

typedef enum {
    BS_RENDER_FLAG_SELECTION,
    BS_RENDER_FLAG_DESTRUCTION
} game_render_flag_t;

// Utils
board_t bs_new_board(void);
void bs_new_board_ptr(board_t* ptr); // Usually just used to clear the board
const char* bs_coords_to_string(Vector2 coords);
int bs_rand(int from, int to);

// Graphics
void bs_render_base_menu(void);
void bs_render_board(board_t* ptr, game_render_flag_t flag);
void bs_render_board_base(int32_t offset_x, int32_t offset_y);
// The "r" variable in these mean either (0) placed, or (1) hovering (selection)
void bs_render_ac(int32_t offset_x, int32_t offset_y, uint8_t r);  // Aicraft carrier
void bs_render_bs(int32_t offset_x, int32_t offset_y, uint8_t r);  // Battleship
void bs_render_ds(int32_t offset_x, int32_t offset_y, uint8_t r);  // Destroyer
void bs_render_sb(int32_t offset_x, int32_t offset_y, uint8_t r);  // Submarine
void bs_render_pb(int32_t offset_x, int32_t offset_y, uint8_t r);  // Patrol Boat

// Functionality
void bs_selection(void);

// Debug
void bs_debug_render(void);

// Bot
void bs_bot_init(bot_t* ptr);

// Colours
#define SEABLUE     CLITERAL(Color){ 0, 105, 148, 255 }
#define UNSELECTED  CLITERAL(Color){ 80, 80, 80, 128 }

// Game definitions
game_state_t bs_state = GAME_STATE_MENU;
board_t* bs_game_board;
bot_t* bs_bot;

bool debug = false;

/// @brief The main function
/// @param argc Args count
/// @param argv Args
/// @return Return code (0 = Success, anything else = issue/error - e.g. 1)
int main(int argc, char* argv[]) {
    InitWindow(800, 450, "BSBOT (Battleship Bot)");
    SetTargetFPS(20); // Doesn't need to be anything good
    SetWindowMinSize(800, 450);

    bs_game_board = malloc(sizeof(board_t));
    bs_bot = malloc(sizeof(bot_t));

    bs_new_board_ptr(bs_game_board);
    bs_bot_init(bs_bot);

    srand(time(0));

    while(!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);

        int w = GetScreenWidth();
        int h = GetScreenHeight();

        if(debug) {
            DrawFPS(10, 10);
            DrawText("BSBOT (Battleship Bot)", w - 10 - (22 * 11), 10, 20, BLUE);
        } else {
            DrawText("BSBOT (Battleship Bot)", 10, 10, 20, BLUE);
        }
        
        switch(bs_state) {
            case GAME_STATE_MENU:
                bs_render_base_menu();

                if(IsKeyPressed(KEY_SPACE)) {
                    bs_state = GAME_STATE_SELECTION;
                }
                break;
            case GAME_STATE_SELECTION:
                bs_render_board(bs_game_board, BS_RENDER_FLAG_SELECTION);
                bs_selection();
                break;
            case GAME_STATE_DESTRUCTION:

                break;
        }

		if(IsKeyPressed(KEY_D)) {
            if(debug) debug = false;
            else debug = true;

            if(debug) {
                SetWindowSize(800, 650);
            } else {
                SetWindowSize(800, 450);
            }
        }

		if (IsKeyPressed(KEY_SPACE)) {
			DrawText("Space pressed!", 300, 300, 20, RED);
		}

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			Vector2 pos = GetMousePosition();
			DrawCircle(pos.x, pos.y, 10, RED);
		}

        DrawText("A project by William Dawson (MrBisquit on GitHub)\thttps://wtdawson.info", 10, h - 20, 15, RAYWHITE);

        if(debug) bs_debug_render();

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

/*
    Function declarations
*/

// Utils
/// @brief Generates a new board
/// @return The new board
board_t bs_new_board(void) {
    board_t b;
    for(uint8_t x = 0; x < 10; x++) {
        for(uint8_t y = 0; y < 10; y++) {
            b.a_places[x][y] = PLACE_BLANK;
            b.b_places[x][y] = PLACE_BLANK;
            b.a_hitmap[x][y] = HIT_BLANK;
            b.b_hitmap[x][y] = HIT_BLANK;
        }
    }
    return b;
}
/// @brief Generates a new board (Or clears an existing one)
/// @param ptr The pointer to the board
void bs_new_board_ptr(board_t* ptr) {
    board_t b = bs_new_board();
    memcpy(ptr, &b, sizeof(board_t));
}

/// @brief Converts Vector2 coodinates to a string (E.g. A1)
/// @param coords Vector2 coordinates
/// @return The string (3 characters)
const char* bs_coords_to_string(Vector2 coords) {
    char* str = malloc(sizeof(char) * 3);
    if(coords.x == 10) {
        const char* text = (char[]){ 'A' + coords.y, '1', '0' };
        strcpy(str, text);
    } else {
        const char* text = (char[]){ 'A' + coords.y, '1' + coords.x + 1, ' ' };
        strcpy(str, text);
    }
    return str;
}

/// @brief Generates a random number between 2 numbers
/// @param from From
/// @param to To
/// @return A number between the 2 numbers provided
int bs_rand(int from, int to) {
    return (rand() % to) + from;
}

// Graphics
/// @brief Renders the main menu
void bs_render_base_menu(void) {
    DrawText("BSBOT (Battleship Bot)", 10, 50, 25, BLUE);
    DrawText("Press Space to start", 10, 100, 15, SKYBLUE);

    if(debug) {
        DrawText("Press D to disable debug", 10, 125, 15, GREEN);
    } else {
        DrawText("Press D to enable debug", 10, 125, 15, RED);
    }
}

/// @brief Renders the board
/// @param ptr The pointer to the board
/// @param flag Any rendering flags
void bs_render_board(board_t* ptr, game_render_flag_t flag) {
    int w = GetScreenWidth();
    int h = GetScreenHeight();

    bs_render_board_base(20, 50);
    if(flag == BS_RENDER_FLAG_DESTRUCTION) bs_render_board_base((w / 2) + 20, 50);
    else {
        DrawText("Select below, then place on the board\non the left. Use your arrow keys, and\npress 'R' to rotate!", (w / 2) + 20, 50, 17, WHITE);

        DrawText("1.\tAircraft Carrier\n2.\tBattleship\n3.\tDestroyer\n4.\tSubmarine\n5.\tPatrol Boat\n\nPress Return (Enter) to continue.", (w / 2) + 20, 125, 15, WHITE);
    }

    DrawLine(w / 2, 50, w / 2, h - 38, WHITE);
}

/// @brief Renders the base of a board
/// @param offset_x X offset (Top-left X coordinate)
/// @param offset_y Y offset (Top-left Y coordinate)
void bs_render_board_base(int32_t offset_x, int32_t offset_y) {
    DrawRectangle(offset_x + 32, offset_y + 32, 320 + 10, 320 + 10, SEABLUE);
    for (uint8_t i = 0; i < 11; i++)
    {
        for(uint8_t j = 0; j < 11; j++) {
            if(i == 0 && j != 0) {
                const char* text = (char[]){ 'A' + j - 1, ' ', ' ', ' ', ' ' };
                DrawText(text, offset_x + 12, offset_y + ((j * 32) + (j * 1)) + 10, 12, WHITE); // Letters
                continue;
            } else if(j == 0 && i != 0) {
                if(i == 10) {
                    DrawText("10", offset_x + ((i * 32) + (i * 1)) + 12, offset_y + 10, 12, WHITE); // Numbers
                } else {
                    const char* text = (char[]){ '1' + i - 1, ' ', ' ', ' ', ' ' };
                    DrawText(text, offset_x + ((i * 32) + (i * 1)) + 12, offset_y + 10, 12, WHITE); // Numbers
                }
                continue;
            }

            DrawRectangle(offset_x + ((i * 32) + (i * 1)), offset_y + ((j * 32) + (j * 1)), 32, 32, UNSELECTED);
        }
    }
}

/// @brief Render an Aircraft carrier
/// @param offset_x X offset (Top-left X coordinate)
/// @param offset_y Y offset (Top-left Y coordinate)
/// @param r (0) placed, or (1) hovering (selection)
void bs_render_ac(int32_t offset_x, int32_t offset_y, uint8_t r) {
    if(r == 0) {

    } else if(r == 1) {

    }
}

/// @brief Render a Battleship
/// @param offset_x X offset (Top-left X coordinate)
/// @param offset_y Y offset (Top-left Y coordinate)
/// @param r (0) placed, or (1) hovering (selection)
void bs_render_bs(int32_t offset_x, int32_t offset_y, uint8_t r) {
    if(r == 0) {

    } else if(r == 1) {
        
    }
}

/// @brief Render a Destroyer
/// @param offset_x X offset (Top-left X coordinate)
/// @param offset_y Y offset (Top-left Y coordinate)
/// @param r (0) placed, or (1) hovering (selection)
void bs_render_ds(int32_t offset_x, int32_t offset_y, uint8_t r) {
    if(r == 0) {

    } else if(r == 1) {
        
    }
}

/// @brief Render a Submarine
/// @param offset_x X offset (Top-left X coordinate)
/// @param offset_y Y offset (Top-left Y coordinate)
/// @param r (0) placed, or (1) hovering (selection)
void bs_render_sb(int32_t offset_x, int32_t offset_y, uint8_t r) {
    if(r == 0) {

    } else if(r == 1) {
        
    }
}


/// @brief Render a Patrol Boat
/// @param offset_x X offset (Top-left X coordinate)
/// @param offset_y Y offset (Top-left Y coordinate)
/// @param r (0) placed, or (1) hovering (selection)
void bs_render_pb(int32_t offset_x, int32_t offset_y, uint8_t r) {
    if(r == 0) {

    } else if(r == 1) {
        
    }
}

// Functionality
/// @brief This is the functionality for the selection
void bs_selection(void) {
    static uint8_t selected_vehicle = 0;
    static Vector2 selected_vec = { .x = 0, .y = 0 };
    static uint8_t selected_rot = 0; // 0 = Horizontal, 1 = Vertical

    if(IsKeyPressed(KEY_ONE)) {
        selected_vehicle = PLACE_AC;
        goto prepare;
    } else if(IsKeyPressed(KEY_TWO)) {
        selected_vehicle = PLACE_BS;
        goto prepare;
    } else if(IsKeyPressed(KEY_THREE)) {
        selected_vehicle = PLACE_DS;
        goto prepare;
    } else if(IsKeyPressed(KEY_FOUR)) {
        selected_vehicle = PLACE_SB;
        goto prepare;
    } else if(IsKeyPressed(KEY_FIVE)) {
        selected_vehicle = PLACE_PB;
        goto prepare;
    }



prepare:
    selected_vec.x = 0;
    selected_vec.y = 0;
    selected_rot = 0;
}

// Debug
void bs_debug_render(void) {
    int w = GetScreenWidth();
    int h = GetScreenHeight();

    DrawRectangle(10, h - 225, w - 20, 200, BLUE);
    int offset_x = 10;
    int offset_y = h - 225;

    DrawText("Debug Mode", offset_x + 10, offset_y + 10, 20, PINK);
    for(uint8_t y = 0; y < 10; y++) {
        for(uint8_t x = 0; x < 10; x++) {
            uint8_t v = (uint8_t)(255 * (uint8_t)(bs_bot->possibilities[x][y] * 100) / 100);
            Color c = (Color){ .r = v, .g = v, .b = v, .a = 255 };

            DrawRectangle(offset_x + 10 + (11 * x), offset_y + 50 + (11 * y), 10, 10, c);
        }
    }
    //DrawText(bs_coords_to_string((Vector2){ .x = 1, .y = 1 }), offset_x + 10, offset_y + 50, 15, PINK);
}

/*
    Bot functionality
*/

/*
    The bot is (probably) the most complex part of this project.
    All 100 (10x10) squares start on a fair possibility (0.5), and (for obvious reasons)
    the player has to place first.

    Unlike what I did for nacbot, which was to basically simulate every possible win and find
    the most likely move that would win and place there, it would be too computationally heavy,
    and time consuming to that for battleship. So it'll be a similar set up, but modifying the
    values as the game progresses.

    The possibilities go up and down based on both boards, since if the player attacks somewhere
    and hits nothing, we can vaguely guess that it may be somewhere around where they placed one
    of theirs. So we increment everything within a 3x3 area of that with some amount. Since it's
    a guess, we don't increment it by much.

    When we hit something, we can use an algorithm to guess where to place next, also incrementing
    values in a sort of 3x3 area. It's actually more of a diamond shape that a square. Since it's
    middle gets set to 0, since it's a hit, meaning ignore that value, then the top, left, right,
    and bottom all increment significantly.
    
    When a ship is destroyed, we can decrement the possibility of everything around it by some
    small amount, since I doubt players are likely to place them directly next to each other.
    (Though this may be a bad idea)
*/

/// @brief Initialise the bot
/// @param ptr The pointer to the bot
void bs_bot_init(bot_t* ptr) {
    memset(ptr, 0, sizeof(bot_t));

    for(uint8_t x = 0; x < 10; x++) {
        for(uint8_t y = 0; y < 10; y++) {
            // Accidentally made a gradient while testing
            //ptr->possibilities[x][y] = ((float)x / (float)18) + ((float)y / (float)18);
            ptr->possibilities[x][y] = 0.5f;
        }
    }
}