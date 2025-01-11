#ifndef SHARED_H
#define SHARED_H

#include <semaphore.h>

#define WIDTH 10
#define HEIGHT 10
#define MAX_FOOD 1
#define GAME_TIME 60
#define SHM_NAME "/snake_shm"

typedef struct {
    int x, y;
} Point;

typedef struct {
    int length;
    Point body[WIDTH * HEIGHT];
} Snake;

typedef struct {
    char gameField[HEIGHT][WIDTH];
    Snake snake;
    Point fruit[MAX_FOOD];
    int game_is_running;
    int score;
    int remaining_time;
    char user_input;
    char game_over_reason[60];
    sem_t sem;
} SharedData;

#endif
