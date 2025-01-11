#ifndef CLIENT_H
#define CLIENT_H

#include "shared.h"

void drawGameField();

void* inputThreadFunc(void* arg);
void* outputThreadFunc(void* arg);

#endif
