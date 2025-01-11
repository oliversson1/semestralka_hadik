#ifndef SERVER_H
#define SERVER_H

#include "shared.h"
#include <semaphore.h>

void initGame();

void *gameTimer(void *arg);

void moveSnake(int dx, int dy);

#endif
