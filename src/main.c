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

#define PLACE_HIT_INVALID 0xFF

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

    // Position (This can be ignored unless used in rendering)
    Vector2 pos;
} item_t;

typedef struct {
    uint8_t a_places[10][10];
    uint8_t b_places[10][10];

    uint8_t a_hitmap[10][10];
    uint8_t b_hitmap[10][10];

    item_t a_items[5];
    item_t b_items[5];
} board_t;

/// @brief Return value for a grid check
typedef struct {
    uint8_t grid[10][10];
    uint8_t total;
} grid_check_return_t;

typedef enum {
    GAME_STATE_MENU,
    GAME_STATE_SELECTION,
    GAME_STATE_DESTRUCTION
} game_state_t;

typedef enum {
    BS_RENDER_FLAG_SELECTION,
    BS_RENDER_FLAG_DESTRUCTION
} game_render_flag_t;

typedef enum {
    BS_Aircraft_Carrier,
    BS_Battleship,
    BS_Destroyer,
    BS_Submarine,
    BS_Patrol_Boat
} game_item_t;

// Utils
board_t bs_new_board(void);
void bs_new_board_ptr(board_t* ptr); // Usually just used to clear the board
const char* bs_coords_to_string(Vector2 coords);
int bs_rand(int from, int to);
grid_check_return_t bs_grid_check(Rectangle rect, uint32_t offset_x, uint32_t offset_y);
item_t bs_get_item(game_item_t type);
bool bs_rect_overlap(Rectangle a, Rectangle b);
bool bs_point_in_rect(Vector2 point, Rectangle rect);
bool bs_add_item(item_t array[5], item_t item);

// Graphics
void bs_render_base_menu(void);
void bs_render_board(board_t* ptr, game_render_flag_t flag);
void bs_render_board_base(int32_t offset_x, int32_t offset_y);
void bs_render_board_selection(uint32_t offset_x, uint32_t offset_y, uint8_t selection[10][10]);
// The "r" variable in these mean either (0) placed, or (1) hovering (selection)
void bs_render_ac(int32_t offset_x, int32_t offset_y, uint8_t r, uint8_t rot);  // Aicraft carrier
void bs_render_bs(int32_t offset_x, int32_t offset_y, uint8_t r, uint8_t rot);  // Battleship
void bs_render_ds(int32_t offset_x, int32_t offset_y, uint8_t r, uint8_t rot);  // Destroyer
void bs_render_sb(int32_t offset_x, int32_t offset_y, uint8_t r, uint8_t rot);  // Submarine
void bs_render_pb(int32_t offset_x, int32_t offset_y, uint8_t r, uint8_t rot);  // Patrol Boat
void bs_render_item(uint8_t type, int32_t offset_x, int32_t offset_y, uint8_t r, uint8_t rot); // Render the selected item

// Functionality
void bs_selection(void);

// Debug
void bs_debug_render(void);

// Bot
void bs_bot_init(bot_t* ptr);

// Colours
#define SEABLUE     CLITERAL(Color){ 0, 105, 148, 255 }
#define UNSELECTED  CLITERAL(Color){ 80, 80, 80, 128 }
#define SELECTED    CLITERAL(Color){ 80, 80, 80, 200 }

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

    for(uint8_t i = 0; i < 5; i++) {
        b.a_items[i].type = PLACE_HIT_INVALID;
        b.b_items[i].type = PLACE_HIT_INVALID;
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

/// @brief Checks which squares in the grid the rectangle is in
/// @param rect The rectangle
/// @param offset_x X offset (Top-left X coordinate)
/// @param offset_y Y offset (Top-left Y coordinate)
/// @return The squares in the grid that the rectangle is in
grid_check_return_t bs_grid_check(Rectangle rect, uint32_t offset_x, uint32_t offset_y) {
    grid_check_return_t grid;
    memset(&grid, 0, sizeof(grid_check_return_t));

    for(uint8_t y = 0; y < 10; y++) {
        for(uint8_t x = 0; x < 10; x++) {
            Rectangle a = {
                .x = offset_x + 33 + (33 * x),
                .y = offset_y + 33 + (33 * y),
                .width = 32,
                .height = 32
            };

            if(bs_rect_overlap(a, rect)) {
                grid.grid[x][y] = true;
                grid.total++;
                //DrawRectangle(a.x, a.y, a.width, a.height, GREEN);
            } else if(bs_point_in_rect((Vector2) { .x = rect.x, .y = rect.y }, a)) {
                grid.grid[x][y] = true;
                grid.total++;
                //DrawRectangle(a.x, a.y, a.width, a.height, GREEN);
            }
        }
    }

    //DrawRectangle(rect.x, rect.y, rect.width + 5, rect.height + 5, BLUE);

    return grid;
}

/// @brief Gets an item
/// @param type The type of item
/// @return The item
item_t bs_get_item(game_item_t type) {
    item_t item;

    switch(type) {
        case BS_Aircraft_Carrier:
            item.type = PLACE_AC;
            item.places = 5;
            item.rotation = 0;
            item.size_normal = (Vector2){ .x = 0, .y = 0 };
            item.size_hovering = (Vector2){ .x = 10, .y = 135 };
            break;
        case BS_Battleship:
            item.type = PLACE_BS;
            item.places = 4;
            item.rotation = 0;
            item.size_hovering = (Vector2){ .x = 0, .y = 0 };
            item.size_normal = (Vector2){ .x = 10, .y = 100 };
            break;
        case BS_Destroyer:
            item.type = PLACE_DS;
            break;
        case BS_Submarine:
            item.type = PLACE_SB;
            break;
        case BS_Patrol_Boat:
            item.type = PLACE_PB;
            break;
    }

    return item;
}

/// @brief Check if 2 rectangles overlap
/// @param a Rectangle A
/// @param b Rectangle B
/// @return If the 2 rectangles overlap
bool bs_rect_overlap(Rectangle a, Rectangle b) {
    float a_x1 = a.x;
    float a_y1 = a.y;
    float a_x2 = a.x + a.width;
    float a_y2 = a.y + a.height;

    float b_x1 = b.x;
    float b_y1 = b.y;
    float b_x2 = b.x + b.width;
    float b_y2 = b.y + b.height;

    /* This only selects the top and bottom squares
    bool tl, tr, bl, br = false;
    tl = bs_point_in_rect((Vector2){ .x = b_x1, .y = b_y1 }, a);
    tr = bs_point_in_rect((Vector2){ .x = b_x2, .y = b_y1 }, a);
    bl = bs_point_in_rect((Vector2){ .x = b_x1, .y = b_y2 }, a);
    br = bs_point_in_rect((Vector2){ .x = b_x2, .y = b_y2 }, a);

    if((tl && tr) || (tl && bl) || (bl && br) || (br && tr))
        return true;
    else
        return false;*/

    /*if(a_x1 < b_x2 && a_x2 > b_x1 && a_y1 > b_y2 && a_y2 < b_y1) {
        return true;
    } else {
        return false;
    }*/

    if(a_x1 <= b_x2 && a_x2 >= b_x1 && a_y1 <= b_y2 && a_y2 >= b_y1) {
        return true;
    } else {
        return false;
    }

    // indicates whether or not the specified rectangle intersects with this rectangle
/*constexpr bool intersects(const rectx& rect) const {
    return (left() <= rect.right() && right()>= rect.left() &&
        top() <= rect.bottom() && bottom() >= rect.top() ); 
}*/
}

/// @brief Check is a point is inside a rectangle
/// @param point The point to check
/// @param rect The rectangle
/// @return If the point is inside the rectangle
bool bs_point_in_rect(Vector2 point, Rectangle rect) {
    float px = point.x;
    float py = point.y;

    float x1 = rect.x;
    float y1 = rect.y;
    float x2 = rect.x + rect.width;
    float y2 = rect.y + rect.height;

    if(px >= x1 && px <= x2 && py >= y1 && py <= y2) {
        return true;
    } else {
        return false;
    }
}

/// @brief Adds an item into the array (if it can fit)
/// @param array The array
/// @param item The item to add into the array
/// @return Returns `true` if it could fit it into the array, `false` if not
bool bs_add_item(item_t array[5], item_t item) {
    for(uint8_t i = 0; i < 5; i++) {
        if(array[i].type == PLACE_HIT_INVALID) {
            memcpy(&array[i], &item, sizeof(item_t));

            return true;
        }
    }

    return false;
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

/// @brief Renders a highlighted selection of the grid (for selection)
/// @note This is designed to be layered on top of `bs_render_board_base`
/// @param offset_x  X offset (Top-left X coordinate)
/// @param offset_y Y offset (Top-left Y coordinate)
/// @param selection The selected grid
void bs_render_board_selection(uint32_t offset_x, uint32_t offset_y, uint8_t selection[10][10]) {
    for(uint8_t y = 0; y < 11; y++) {
        for(uint8_t x = 0; x < 11; x++) {
            if(y == 0 || x == 0) continue; // Skip

            if(selection[y - 1][x - 1] == true) {
                DrawRectangle(offset_x + ((y * 32) + (y * 1)), offset_y + ((x * 32) + (x * 1)), 32, 32, SELECTED);
            }
        }
    }
}

/// @brief Render an Aircraft carrier
/// @param offset_x X offset (Top-left X coordinate)
/// @param offset_y Y offset (Top-left Y coordinate)
/// @param r (0) placed, or (1) hovering (selection)
/// @param rot Rotation (0 = Horizontal, 1 = Vertical)
void bs_render_ac(int32_t offset_x, int32_t offset_y, uint8_t r, uint8_t rot) {
    item_t item = bs_get_item(BS_Aircraft_Carrier);
    if(r == 0) {
        uint8_t width = item.size_normal.x;
        uint8_t height = item.size_normal.y;
        if(rot == 0)
            DrawRectangle(offset_x, offset_y, width, height, RED);
        else
            DrawRectangle(offset_x, offset_y, height, width, RED);
    } else if(r == 1) {
        uint8_t width = item.size_hovering.x;
        uint8_t height = item.size_hovering.y;
        if(rot == 0)
            DrawRectangle(offset_x, offset_y, width, height, RED);
        else
            DrawRectangle(offset_x, offset_y, height, width, RED);
    }
}

/// @brief Render a Battleship
/// @param offset_x X offset (Top-left X coordinate)
/// @param offset_y Y offset (Top-left Y coordinate)
/// @param r (0) placed, or (1) hovering (selection)
/// @param rot Rotation (0 = Horizontal, 1 = Vertical)
void bs_render_bs(int32_t offset_x, int32_t offset_y, uint8_t r, uint8_t rot) {
    item_t item = bs_get_item(BS_Battleship);
    if(r == 0) {
        uint8_t width = item.size_normal.x;
        uint8_t height = item.size_normal.y;
        if(rot == 0)
            DrawRectangle(offset_x, offset_y, width, height, RED);
        else
            DrawRectangle(offset_x, offset_y, height, width, RED);
    } else if(r == 1) {
        uint8_t width = item.size_hovering.x;
        uint8_t height = item.size_hovering.y;
        if(rot == 0)
            DrawRectangle(offset_x, offset_y, width, height, RED);
        else
            DrawRectangle(offset_x, offset_y, height, width, RED);
    }
}

/// @brief Render a Destroyer
/// @param offset_x X offset (Top-left X coordinate)
/// @param offset_y Y offset (Top-left Y coordinate)
/// @param r (0) placed, or (1) hovering (selection)
/// @param rot Rotation (0 = Horizontal, 1 = Vertical)
void bs_render_ds(int32_t offset_x, int32_t offset_y, uint8_t r, uint8_t rot) {
    item_t item = bs_get_item(BS_Destroyer);
    if(r == 0) {
        uint8_t width = item.size_normal.x;
        uint8_t height = item.size_normal.y;
        if(rot == 0)
            DrawRectangle(offset_x, offset_y, width, height, RED);
        else
            DrawRectangle(offset_x, offset_y, height, width, RED);
    } else if(r == 1) {
        uint8_t width = item.size_hovering.x;
        uint8_t height = item.size_hovering.y;
        if(rot == 0)
            DrawRectangle(offset_x, offset_y, width, height, RED);
        else
            DrawRectangle(offset_x, offset_y, height, width, RED);
    }
}

/// @brief Render a Submarine
/// @param offset_x X offset (Top-left X coordinate)
/// @param offset_y Y offset (Top-left Y coordinate)
/// @param r (0) placed, or (1) hovering (selection)
/// @param rot Rotation (0 = Horizontal, 1 = Vertical)
void bs_render_sb(int32_t offset_x, int32_t offset_y, uint8_t r, uint8_t rot) {
    item_t item = bs_get_item(BS_Submarine);
    if(r == 0) {
        uint8_t width = item.size_normal.x;
        uint8_t height = item.size_normal.y;
        if(rot == 0)
            DrawRectangle(offset_x, offset_y, width, height, RED);
        else
            DrawRectangle(offset_x, offset_y, height, width, RED);
    } else if(r == 1) {
        uint8_t width = item.size_hovering.x;
        uint8_t height = item.size_hovering.y;
        if(rot == 0)
            DrawRectangle(offset_x, offset_y, width, height, RED);
        else
            DrawRectangle(offset_x, offset_y, height, width, RED);
    }
}


/// @brief Render a Patrol Boat
/// @param offset_x X offset (Top-left X coordinate)
/// @param offset_y Y offset (Top-left Y coordinate)
/// @param r (0) placed, or (1) hovering (selection)
/// @param rot Rotation (0 = Horizontal, 1 = Vertical)
void bs_render_pb(int32_t offset_x, int32_t offset_y, uint8_t r, uint8_t rot) {
    item_t item = bs_get_item(BS_Patrol_Boat);
    if(r == 0) {
        uint8_t width = item.size_normal.x;
        uint8_t height = item.size_normal.y;
        if(rot == 0)
            DrawRectangle(offset_x, offset_y, width, height, RED);
        else
            DrawRectangle(offset_x, offset_y, height, width, RED);
    } else if(r == 1) {
        uint8_t width = item.size_hovering.x;
        uint8_t height = item.size_hovering.y;
        if(rot == 0)
            DrawRectangle(offset_x, offset_y, width, height, RED);
        else
            DrawRectangle(offset_x, offset_y, height, width, RED);
    }
}

/// @brief Renders the selected item
/// @param type The item type to render
/// @param offset_x X offset (Top-left X coordinate)
/// @param offset_y Y offset (Top-left Y coordinate)
/// @param r (0) placed, or (1) hovering (selection)
/// @param rot Rotation (0 = Horizontal, 1 = Vertical)
void bs_render_item(uint8_t type, int32_t offset_x, int32_t offset_y, uint8_t r, uint8_t rot) {
    switch(type) {
        case 1:
            bs_render_ac(offset_x, offset_y, r, rot);
            break;
        case 2:
            bs_render_bs(offset_x, offset_y, r, rot);
            break;
        case 3:
            bs_render_ds(offset_x, offset_y, r, rot);
            break;
        case 4:
            bs_render_sb(offset_x, offset_y, r, rot);
            break;
        case 5:
            bs_render_pb(offset_x, offset_y, r, rot);
            break;
    }
}

// Functionality
/// @brief This is the functionality for the selection
void bs_selection(void) {
    static uint8_t selected_vehicle = 0;
    static Vector2 selected_vec = { .x = 0, .y = 0 };
    static uint8_t selected_rot = 0; // 0 = Horizontal, 1 = Vertical
    static item_t item;

    if(IsKeyPressed(KEY_ONE)) {
        selected_vehicle = PLACE_AC;
        item = bs_get_item(BS_Aircraft_Carrier);
        goto prepare;
    } else if(IsKeyPressed(KEY_TWO)) {
        selected_vehicle = PLACE_BS;
        item = bs_get_item(BS_Battleship);
        goto prepare;
    } else if(IsKeyPressed(KEY_THREE)) {
        selected_vehicle = PLACE_DS;
        item = bs_get_item(BS_Destroyer);
        goto prepare;
    } else if(IsKeyPressed(KEY_FOUR)) {
        selected_vehicle = PLACE_SB;
        item = bs_get_item(BS_Submarine);
        goto prepare;
    } else if(IsKeyPressed(KEY_FIVE)) {
        selected_vehicle = PLACE_PB;
        item = bs_get_item(BS_Patrol_Boat);
        goto prepare;
    } else if(IsKeyPressed(KEY_R)) {
        if(selected_rot == 0) {
            selected_rot = 1;
            item.rotation = 1;
        } else {
            selected_rot = 0;
            item.rotation = 0;
        }
    } else if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        bs_add_item(bs_game_board->a_items, item);

        selected_vehicle = 0;
        goto prepare;
    }

    // Render any pre-existing items on the board
    for(uint8_t i = 0; i < 5; i++) {
        if(bs_game_board->a_items[i].type != PLACE_HIT_INVALID) {
            Rectangle _rect = (Rectangle) {
                .x = bs_game_board->a_items[i].pos.x,
                .y = bs_game_board->a_items[i].pos.y,
                .width = item.size_normal.x,
                .width = item.size_normal.y
            };

            if(selected_rot) {
                _rect.x = bs_game_board->a_items[i].pos.y;
                _rect.y = bs_game_board->a_items[i].pos.x;
                _rect.width = bs_game_board->a_items[i].size_normal.y;
                _rect.height = bs_game_board->a_items[i].size_normal.x;
            }

            grid_check_return_t _result = bs_grid_check(_rect, 20, 50);
            bs_render_board_selection(20, 50, _result.grid);
        }
    }

    // TODO: Fix rotation (Vertical mode)

    int cx = GetMouseX();
    int cy = GetMouseY();

    uint32_t offset_x = cx - (item.size_hovering.x / 2);
    uint32_t offset_y = cy - (item.size_hovering.y / 2);

    Rectangle rect = (Rectangle) {
        .x = cx - (item.size_hovering.x / 2),
        .y = cy - (item.size_hovering.y / 2),
        .width = item.size_hovering.x,
        .height = item.size_hovering.y
    };

    if(selected_rot == 1) {
        rect.x = cy - (item.size_hovering.y / 2);
        rect.y = cx - (item.size_hovering.x / 2);
        rect.width = item.size_hovering.y;
        rect.height = item.size_hovering.x;

        //offset_x = cy - (item.size_hovering.y / 2);
        //offset_y = cx - (item.size_hovering.x / 2);
    }

    item.pos.x = rect.x;
    item.pos.y = rect.y;

    grid_check_return_t result = bs_grid_check(rect, 20, 50);
    bs_render_board_selection(20, 50, result.grid);

    bs_render_item(selected_vehicle, offset_x, offset_y, 1, selected_rot);

    goto end;

prepare:
    selected_vec.x = 0;
    selected_vec.y = 0;
    selected_rot = 0;

end:
    return;
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