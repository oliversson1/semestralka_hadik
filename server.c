#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

#include "shared.h"
#define SHM_NAME "/snake_shm"
SharedData *sharedData = NULL;

void initGame() {
    sem_wait(&sharedData->sem);

    sharedData->snake.length = 1;
    sharedData->snake.body[0].x = WIDTH / 2;
    sharedData->snake.body[0].y = HEIGHT / 2;
    sharedData->score = 0;
    sharedData->game_is_running = 1;
    sharedData->user_input = '\0';
    sharedData->remaining_time = GAME_TIME;

    for (int i = 0; i < MAX_FOOD; i++) {
        sharedData->fruit[i].x = rand() % (WIDTH - 2) + 1;
        sharedData->fruit[i].y = rand() % (HEIGHT - 2) + 1;
    }

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            sharedData->gameField[y][x] = ' ';
        }
    }

    sharedData->gameField[sharedData->snake.body[0].y][sharedData->snake.body[0].x] = '*';
    for (int i = 0; i < MAX_FOOD; i++) {
        sharedData->gameField[sharedData->fruit[i].y][sharedData->fruit[i].x] = 'O';
    }

    sem_post(&sharedData->sem);
}

void *gameTimer(void *arg) {
    while (1) {
        sleep(1);

        sem_wait(&sharedData->sem);
        if (sharedData->remaining_time > 0) {
            sharedData->remaining_time--;
            if (sharedData->remaining_time <= 0) {
                sharedData->game_is_running = 0;
                strcpy(sharedData->game_over_reason, "Game Over! Time is up.\n");
                sem_post(&sharedData->sem);
                break;
            }
        }
        sem_post(&sharedData->sem);
    }
    return NULL;
}

void moveSnake(int dx, int dy) {
    for (int i = 0; i < sharedData->snake.length; i++) {
        sharedData->gameField[sharedData->snake.body[i].y][sharedData->snake.body[i].x] = ' ';
    }


    for (int i = sharedData->snake.length - 1; i > 0; i--) {
        sharedData->snake.body[i] = sharedData->snake.body[i - 1];
    }
    sharedData->snake.body[0].x += dx;
    sharedData->snake.body[0].y += dy;

    int hx = sharedData->snake.body[0].x;
    int hy = sharedData->snake.body[0].y;


    if (hx <= 0 || hx >= WIDTH - 1 || hy <= 0 || hy >= HEIGHT - 1) {
        sharedData->game_is_running = 0;
        strcpy(sharedData->game_over_reason, "Game Over! Snake hit the wall.\n");
        return;
    }

    for (int i = 1; i < sharedData->snake.length; i++) {
        if (hx == sharedData->snake.body[i].x && hy == sharedData->snake.body[i].y) {
            sharedData->game_is_running = 0;
            strcpy(sharedData->game_over_reason, "Game Over! Snake collided with itself.\n");
            return;
        }
    }


    for (int i = 0; i < MAX_FOOD; i++) {
        if (hx == sharedData->fruit[i].x && hy == sharedData->fruit[i].y) {
            sharedData->snake.length++;
            sharedData->score += 10;
            sharedData->fruit[i].x = rand() % (WIDTH - 2) + 1;
            sharedData->fruit[i].y = rand() % (HEIGHT - 2) + 1;
        }
    }

    for (int i = 0; i < sharedData->snake.length; i++) {
        sharedData->gameField[sharedData->snake.body[i].y][sharedData->snake.body[i].x] = '*';
    }

    for (int i = 0; i < MAX_FOOD; i++) {
        sharedData->gameField[sharedData->fruit[i].y][sharedData->fruit[i].x] = 'O';
    }
}

int main() {
    srand(time(NULL));

    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd < 0) {
        perror("shm_open");
        return 1;
    }

    if (ftruncate(fd, sizeof(*sharedData)) == -1) {
        perror("ftruncate");
        close(fd);
        return 1;
    }

    sharedData = mmap(NULL, sizeof(*sharedData), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (sharedData == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }
    close(fd);

    if (sem_init(&sharedData->sem, 1, 1) != 0) {
        perror("sem_init");
        munmap(sharedData, sizeof(*sharedData));
        return 1;
    }

    initGame();

    pthread_t timerThread;
    pthread_create(&timerThread, NULL, gameTimer, NULL);

    while (1) {
        sem_wait(&sharedData->sem);

        char c = sharedData->user_input;
        if (c != '\0') {
            if (c == 'q') {
                sharedData->game_is_running = 0;
                sem_post(&sharedData->sem);
                break;
            }
            int dx = 0, dy = 0;
            if (c == 'w') { dx = 0; dy = -1; }
            if (c == 'a') { dx = -1; dy = 0; }
            if (c == 's') { dx = 0; dy = 1; }
            if (c == 'd') { dx = 1; dy = 0; }

            if (dx != 0 || dy != 0) {
                moveSnake(dx, dy);
            }

            sharedData->user_input = '\0';
        }

        sem_post(&sharedData->sem);

        usleep(200000);
    }

    pthread_join(timerThread, NULL);

    sem_destroy(&sharedData->sem);

    munmap(sharedData, sizeof(*sharedData));
    shm_unlink(SHM_NAME);

    return 0;
}
