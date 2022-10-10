#include "primlib.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "pieces.inl"

#define MAX_TETRINOS 100
#define TETRINO_SIZE 20
#define BOX_TOP_Y 150
#define BOX_TOP_X gfx_screenWidth() / 2
#define ROWS 26
#define COLLUMNS 10
#define CONTROLS_DELAY 7
#define DELAY 10
#define FALL_DELAY 10

int BOX_HEIGHT = ROWS * TETRINO_SIZE;
int BOX_WIDTH = COLLUMNS * TETRINO_SIZE;

char gamebox[ROWS][COLLUMNS] = {0};

typedef struct {
    int x_id;
    int y_id;
    int type;
    int rotation;
} Tetrino;

typedef struct {
    int top;
    Tetrino data[MAX_TETRINOS];
} Stack;

typedef struct {
    Stack tetrinos;
    Tetrino tet[MAX_TETRINOS];
    Tetrino *falling;
    Tetrino *next;
} Context;

void push(Stack *, Tetrino *);
Tetrino *pop(Stack *);
Tetrino *peek(Stack *);
void main_menu();
void lose_screen();
void win_screen();
void draw_gamebox();
void draw_next(Context *);
void create_falling_tetrino(Context *);
void draw_tetrinos();
void draw_falling_tetrino(Context *);
void draw_next_tetrino(Context *);
void move_down_fast(Context *);
void move_tetrino_down(Context *, int);
void move_tetrino_side(Context *);
int find_boundary_x(Context *);
int find_boundary_y(Context *);
void rotate_tetrino(Context *);
int can_rotate(Context *);
void put_fallen(Context *);
int check_collision(Context *);
void correct_axis(Context *);
void rev_correct_axis(Context *);
int check_row();
int check_lose();
int check_win();
void delete_row(int);
void init_tetrinos(Context *);
void init_context(Context *);
void run(Context *);

void push(Stack *stack, Tetrino *tetrino) {
	if (stack->top < MAX_TETRINOS)
		stack->data[stack->top++] = *tetrino;
}

Tetrino *pop(Stack *stack) {
    if (stack->top > 0) 
    	return &stack->data[--stack->top];
	else
		return NULL;
}

Tetrino *peek(Stack *stack) {
    if (stack->top > 0)
    	return &stack->data[stack->top - 1];
	else
		return NULL;
}

void draw_gamebox() {
    int x = BOX_TOP_X;
    int y = BOX_TOP_Y + 5 * TETRINO_SIZE;

    gfx_line(x - 1, y, x - 1, y + BOX_HEIGHT - 5 * TETRINO_SIZE + 1, WHITE);
    gfx_line(x - 1, y + BOX_HEIGHT - 5 * TETRINO_SIZE + 1, x + BOX_WIDTH + 1, y + BOX_HEIGHT - 5 * TETRINO_SIZE + 1, WHITE);
    gfx_line(x + BOX_WIDTH + 1, y + BOX_HEIGHT - 5 * TETRINO_SIZE + 1, x + BOX_WIDTH + 1, y, WHITE);
}

void run(Context *context) {
    int run = 1;
    int counter = 0;

    while (run == 1) {
        gfx_filledRect(0, 0, gfx_screenWidth() - 1, gfx_screenHeight() - 1, BLACK);
        draw_gamebox();
        draw_tetrinos();
        draw_falling_tetrino(context);
        draw_next_tetrino(context);
        move_tetrino_down(context, counter);
        move_tetrino_side(context);
        rotate_tetrino(context);
        move_down_fast(context);
        int row = check_row();
        if (row != -1) {
            delete_row(row);
        }
        gfx_updateScreen();
        SDL_Delay(DELAY);
        if (context->falling == NULL && peek(&context->tetrinos) != NULL) {
            create_falling_tetrino(context);
        }
        if(check_lose() == 1) {
            lose_screen();
            run = 0;
        }
        else if (check_win(context) == 1 && context->falling == NULL) {
            win_screen();
            run = 0;
        }

        if (gfx_isKeyDown(SDLK_ESCAPE) == 1) {
            run = 0;
        }
        counter++;
    }
}

void main_menu() {
    gfx_filledRect(0, 0, gfx_screenWidth() - 1, gfx_screenHeight() - 1, BLACK);
    gfx_textout((gfx_screenWidth() / 2 - 40), gfx_screenHeight() / 5, "TETRIS", RED);
    gfx_textout((gfx_screenWidth() / 2 - 100), gfx_screenHeight() / 2, "PRESS ANY KEY TO START", RED);
    gfx_updateScreen();
    gfx_getkey();
}

void create_falling_tetrino(Context *context) {
    context->falling = pop(&context->tetrinos);
    context->falling->x_id = COLLUMNS / 2;
    context->falling->y_id = 1;
}

void draw_tetrinos() {
    for(int row = 0; row < (BOX_HEIGHT / TETRINO_SIZE); row++) {
        for(int col = 0; col < (BOX_WIDTH / TETRINO_SIZE); col++) {
            if (gamebox[row][col] != 0) {
                int x = BOX_TOP_X + (TETRINO_SIZE * col);
                int y = BOX_TOP_Y + (TETRINO_SIZE * row);
                gfx_filledRect(x, y, x + TETRINO_SIZE, y + TETRINO_SIZE, RED);
            }
        }
    }
}

void draw_falling_tetrino(Context *context) {
    if (context->falling != NULL) {
        for (int idy_ct = 0; idy_ct < 4; idy_ct++) {
            for (int idx_ct = 0; idx_ct < 4; idx_ct++) {
                int x = (idx_ct * TETRINO_SIZE) + (context->falling->x_id * TETRINO_SIZE) + BOX_TOP_X;
                int y = (idy_ct * TETRINO_SIZE) + (context->falling->y_id * TETRINO_SIZE) + BOX_TOP_Y;
                if (pieces[context->falling->type][context->falling->rotation][idy_ct][idx_ct] == 1) {
                    gfx_filledRect(x, y, x + TETRINO_SIZE, y + TETRINO_SIZE, GREEN);
                }
                else if (pieces[context->falling->type][context->falling->rotation][idy_ct][idx_ct] == 2) {
                    gfx_filledRect(x, y, x + TETRINO_SIZE, y + TETRINO_SIZE, YELLOW);
                }
            }
        }
    }
}

void draw_next_tetrino(Context *context) {
    if (peek(&context->tetrinos) != NULL) {
        context->next = peek(&context->tetrinos);
        context->next->x_id = -7;
        context->next->y_id = 10;
        for (int idy_ct = 0; idy_ct < 4; idy_ct++) {
            for (int idx_ct = 0; idx_ct < 4; idx_ct++) {
                int x = (idx_ct * TETRINO_SIZE) + (context->next->x_id * TETRINO_SIZE) + BOX_TOP_X;
                int y = (idy_ct * TETRINO_SIZE) + (context->next->y_id * TETRINO_SIZE) + BOX_TOP_Y;
                if (pieces[context->next->type][context->next->rotation][idy_ct][idx_ct] != 0) {
                    gfx_filledRect(x, y, x + TETRINO_SIZE, y + TETRINO_SIZE, GREEN);
                }
            }
        }
    }
}

void rotate_tetrino(Context *context) {
    if (context->falling != NULL) {
        static int is_pressed = 0;
        if (gfx_isKeyDown(SDLK_SPACE) & !is_pressed) {
            is_pressed = 1;
            context->falling->rotation++;
            if (context->falling->rotation > 3) {
                context->falling->rotation = 0;
            }
            correct_axis(context);
            if (can_rotate(context) == 0) {
                context->falling->rotation--;
                if (context->falling->rotation < 0) {
                    context->falling->rotation = 3;
                }
                rev_correct_axis(context);
            }
        }
        else if (!gfx_isKeyDown(SDLK_SPACE)) {
            is_pressed = 0;
        }
    }
}

int can_rotate(Context *context) {
    int bound_x = find_boundary_x(context);
    int bound_y = find_boundary_y(context);
    if (context->falling->x_id < 0 || context->falling->x_id > COLLUMNS - bound_x || context->falling->y_id > ROWS - bound_y || check_collision(context) || context->falling->y_id <= 0) {
        return 0;
    }
    else {
        return 1;
    }
}

void correct_axis(Context *context) {
    int x, x_next, y, y_next, rotation = context->falling->rotation;
    for (int idy_ct = 0; idy_ct < 4; idy_ct++) {
        for (int idx_ct = 0; idx_ct < 4; idx_ct++) {
            if (pieces[context->falling->type][rotation][idy_ct][idx_ct] == 2) {
                x_next = idx_ct;
                y_next = idy_ct;
            }

            if (rotation > 0) {
                if (pieces[context->falling->type][rotation - 1][idy_ct][idx_ct] == 2) {
                    x = idx_ct;
                    y = idy_ct;
                }
            }
            else if (rotation == 0) {
                if (pieces[context->falling->type][3][idy_ct][idx_ct] == 2) {
                    x = idx_ct;
                    y = idy_ct;
                }
            }
        }
    }
    context->falling->x_id = context->falling->x_id + (x - x_next);
    context->falling->y_id = context->falling->y_id + (y - y_next); 
}

void rev_correct_axis(Context *context) {
    int x, x_next, y, y_next, rotation = context->falling->rotation;
    for (int idy_ct = 0; idy_ct < 4; idy_ct++) {
        for (int idx_ct = 0; idx_ct < 4; idx_ct++) {
            if (pieces[context->falling->type][rotation][idy_ct][idx_ct] == 2) {
                x_next = idx_ct;
                y_next = idy_ct;
            }

            if (rotation < 3) {
                if (pieces[context->falling->type][rotation + 1][idy_ct][idx_ct] == 2) {
                    x = idx_ct;
                    y = idy_ct;
                }
            }
            else if (rotation == 3) {
                if (pieces[context->falling->type][0][idy_ct][idx_ct] == 2) {
                    x = idx_ct;
                    y = idy_ct;
                }
            }
        }
    }
    context->falling->x_id = context->falling->x_id + (x - x_next);
    context->falling->y_id = context->falling->y_id + (y - y_next); 
}

void move_tetrino_down(Context *context, int counter) {
    if (context->falling != NULL && counter % FALL_DELAY == 0) {
        int bound = find_boundary_y(context);
        context->falling->y_id++;
        if (context->falling->y_id > ROWS - bound) {
            context->falling->y_id = ROWS - bound;
            put_fallen(context);
        }
        else if (check_collision(context) == 1) {
            context->falling->y_id--;
            put_fallen(context);
        }
    }
}

void move_down_fast(Context *context) {
    static int is_pressed = 0;
    if (gfx_isKeyDown(SDLK_DOWN) && !is_pressed) {
        is_pressed = 1;
        while (context->falling != NULL) {    
            if (context->falling != NULL) {
                int bound = find_boundary_y(context);
                context->falling->y_id++;
                if (context->falling->y_id > ROWS - bound) {
                    context->falling->y_id = ROWS - bound;
                    put_fallen(context);
                }
                else if (check_collision(context) == 1) {
                    context->falling->y_id--;
                    put_fallen(context);
                }
            }
        }
    }
    else if (!gfx_isKeyDown(SDLK_DOWN)) {
        is_pressed = 0;
    }    
}

int find_boundary_y(Context *context) {
    int boundary = 0;
    for (int idx_ct = 0; idx_ct < 4; idx_ct++) {
        for (int idy_ct = 0; idy_ct < 4; idy_ct++) {
            if (pieces[context->falling->type][context->falling->rotation][idy_ct][idx_ct] != 0) {
                if (idy_ct == 3) {
                    boundary = idy_ct + 1;
                }
                else if (pieces[context->falling->type][context->falling->rotation][idy_ct + 1][idx_ct] == 0) {
                    if (boundary < idy_ct + 1) {
                        boundary = idy_ct + 1;
                    }
                }
            }
        }
    }
    
    return boundary;
}

void move_tetrino_side(Context *context) {
    static int left_delay = CONTROLS_DELAY, right_delay = CONTROLS_DELAY;
    if (context->falling != NULL) {
        int boundary = find_boundary_x(context);
        if (gfx_isKeyDown(SDLK_LEFT) == 1) {
            left_delay --;
            if (left_delay == 0) {
                context->falling->x_id--;
                if (context->falling->x_id < 0) {
                    context->falling->x_id = 0;
                }
                if (check_collision(context) == 1) {
                    context->falling->x_id++;
                }
                left_delay = CONTROLS_DELAY;
            }
        }

        if (gfx_isKeyDown(SDLK_RIGHT) == 1) {
            right_delay--;
            if (right_delay == 0) {
                context->falling->x_id++;
                if (context->falling->x_id > COLLUMNS - boundary) {
                    context->falling->x_id = COLLUMNS - boundary;
                }
                if (check_collision(context) == 1) {
                    context->falling->x_id--;
                }
                right_delay = CONTROLS_DELAY;
            }
        }
    }
}

int find_boundary_x(Context *context) {
    int boundary = 0;
    for (int idy_ct = 0; idy_ct < 4; idy_ct++) {
        for (int idx_ct = 0; idx_ct < 4; idx_ct++) {
            if (pieces[context->falling->type][context->falling->rotation][idy_ct][idx_ct] != 0) {
                if (idx_ct == 3) {
                    boundary = idx_ct + 1;
                }
                else if (pieces[context->falling->type][context->falling->rotation][idy_ct][idx_ct + 1] == 0) {
                    if (boundary < idx_ct + 1) {
                        boundary = idx_ct + 1;
                    }
                }
            }
        }
    }

    return boundary;
}

void put_fallen(Context *context) {
    for (int idy_ct = 0; idy_ct < 4; idy_ct++) {
        for (int idx_ct = 0; idx_ct < 4; idx_ct++) {
            if (pieces[context->falling->type][context->falling->rotation][idy_ct][idx_ct] != 0) {
                gamebox[context->falling->y_id + idy_ct][context->falling->x_id + idx_ct] = 1;
            }
        }
    }
    context->falling = NULL;
}

int check_collision(Context *context) {
    for (int idy_ct = 0; idy_ct < 4; idy_ct++) {
        for (int idx_ct = 0; idx_ct < 4; idx_ct++) {
            if (context->falling->x_id + idx_ct < COLLUMNS && context->falling->y_id + idy_ct < ROWS 
                && gamebox[context->falling->y_id + idy_ct][context->falling->x_id + idx_ct] == 1) {
                if (pieces[context->falling->type][context->falling->rotation][idy_ct][idx_ct] != 0) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

int check_row() {
    for (int idy_ct = 0; idy_ct < ROWS; idy_ct++) {
        int counter = 0;
        for (int idx_ct = 0; idx_ct < COLLUMNS; idx_ct++) {
            if (gamebox[idy_ct][idx_ct] == 1) {
                counter++;
            }
            if (counter == COLLUMNS) {
                return idy_ct;
            }
        }
    }
    return -1;
}

void delete_row(int row) {
    for (;row >= 0; row--) {
        for (int idx_ct = 0; idx_ct < COLLUMNS; idx_ct++) {
            if (row > 0) {
                gamebox[row][idx_ct] = gamebox[row - 1][idx_ct];
            }
            else {
                gamebox[row][idx_ct] = 0;
            }
        }
    }
}

void init_tetrinos(Context *context) {
    int lower = 0, upper_t = 6, upper_r = 3;
    for (int tetrino_ct = 0; tetrino_ct < MAX_TETRINOS; tetrino_ct++) {
        Tetrino *tetrino = &context->tet[tetrino_ct];
        tetrino->rotation = (rand() % (upper_r - lower + 1)) + lower;
        tetrino->type = (rand() % (upper_t - lower + 1)) + lower;

        push(&context->tetrinos, tetrino);
    }
}

int check_lose() {
    for(int idx_ct = 0; idx_ct < COLLUMNS; idx_ct++) {
        if (gamebox[3][idx_ct] != 0) {
            return 1;
        }
    }
    return 0;
}

int check_win(Context *context) {
    if (peek(&context->tetrinos) == NULL) {
        return 1;
    }
    else {
        return 0;
    }
}

void lose_screen() {
    gfx_filledRect(0, 0, gfx_screenWidth() - 1, gfx_screenHeight() - 1, BLACK);
    gfx_textout((gfx_screenWidth() / 2 - 40), gfx_screenHeight() / 2, "YOU LOST", RED);
    gfx_updateScreen();
    gfx_getkey();
}

void win_screen() {
    gfx_filledRect(0, 0, gfx_screenWidth() - 1, gfx_screenHeight() - 1, BLACK);
    gfx_textout((gfx_screenWidth() / 2 - 40), gfx_screenHeight() / 2, "YOU WON!", GREEN);
    gfx_updateScreen();
    gfx_getkey();
}

void init_context(Context *context) {
    context->falling = NULL;
}

int main(int argc, char *argv[]) {
    srand(time(0));
    if (gfx_init())
        exit(3);

    main_menu();

    Context context;
    init_context(&context);
    init_tetrinos(&context);

    run(&context);
    return 0;
}