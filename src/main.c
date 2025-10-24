/*
    BSBOT made by William Dawson (MrBisquit on GitHub)
    https://wtdawson.info

    --------------------------------------------------------------------------------------------

    This is my battleship (the game) bot, entirely written in C.
    It was inspired by my other bot, nacbot.

    The ONLY dependencies are:
    *   memory.h
    *   stdio.h
    *   stdint.h
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
#include <stdint.h>
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

// Graphics
void bs_render_base_menu(void);
void bs_render_board(board_t* ptr, game_render_flag_t flag);
void bs_render_board_base(int32_t offset_x, int32_t offset_y);

// Colours
#define SEABLUE     CLITERAL(Color){ 0, 105, 148, 255 }
#define UNSELECTED  CLITERAL(Color){ 80, 80, 80, 128 }

// Game definitions
game_state_t bs_state = GAME_STATE_MENU;
board_t* bs_game_board;

bool debug = false;

/// @brief The main function
/// @param argc Args count
/// @param argv Args
/// @return Return code (0 = Success, anything else = issue/error - e.g. 1)
int main(int argc, char* argv[]) {
    InitWindow(800, 450, "BSBOT (Battleship Bot)");
    SetTargetFPS(20); // Doesn't need to be anything good
    SetWindowMinSize(800, 450);
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
                break;
            case GAME_STATE_DESTRUCTION:

                break;
        }

		if(IsKeyPressed(KEY_D)) {
            if(debug) debug = false;
            else debug = true;
        }

		if (IsKeyPressed(KEY_SPACE)) {
			DrawText("Space pressed!", 300, 300, 20, RED);
		}

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			Vector2 pos = GetMousePosition();
			DrawCircle(pos.x, pos.y, 10, RED);
		}

        DrawText("A project by William Dawson (MrBisquit on GitHub)\thttps://wtdawson.info", 10, h - 20, 15, RAYWHITE);

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