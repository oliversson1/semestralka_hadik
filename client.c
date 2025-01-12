#include "shared.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

SharedData *sharedData = NULL;

void drawGameField() {
    char localGameField[HEIGHT][WIDTH];

    if (sem_wait(&sharedData->sem) != 0) {
        perror("sem_wait");
        return;
    }
    memcpy(localGameField, sharedData->gameField, sizeof(localGameField));
    if (sem_post(&sharedData->sem) != 0) {
        perror("sem_post");
    }

    printf("\033[H\033[J");

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (x == 0 || x == WIDTH - 1 || y == 0 || y == HEIGHT - 1) {
                printf("#");
            } else {
                printf("%c", localGameField[y][x]);
            }
        }
        printf("\n");
    }
    printf("Score: %d | Time left: %d\n", sharedData->score, sharedData->remaining_time);
    printf("Enter move (w/a/s/d) or 'q' to exit game:\n ");
    fflush(stdout);
}

void* inputThreadFunc(void* arg) {
    while (1) {
        if (!sharedData->game_is_running) break;

        char c = getchar();
        if (c == EOF || c == '\n') {
            continue;
        }

        if (sem_wait(&sharedData->sem) != 0) {
            perror("sem_wait");
            continue;
        }

        if (c == 'q') {
            sharedData->game_is_running = 0;
            if (sem_post(&sharedData->sem) != 0) {
                perror("sem_post");
            }
            break;
        }

        if (c == 'w' || c == 'a' || c == 's' || c == 'd') {
            sharedData->user_input = c;
        }

        if (sem_post(&sharedData->sem) != 0) {
            perror("sem_post");
        }
    }
    return NULL;
}

void* outputThreadFunc(void* arg) {
    while (1) {
        if (sem_wait(&sharedData->sem) != 0) {
            perror("sem_wait");
            break;
        }

        int running = sharedData->game_is_running;
        if (sem_post(&sharedData->sem) != 0) {
            perror("sem_post");
            break;
        }

        if (!running) {
            break;
        }

        drawGameField();
        usleep(200000);
    }
    return NULL;
}

void startServer() {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork failed");
        exit(1);
    }

    if (pid == 0) {
        printf("Starting the server...\n");

        int result = execl("./server", "server", NULL);
        if (result == -1) {
            perror("execl failed");
            exit(1);
        }
    } else {
        printf("Server process created successfully.\n");
    }
}

int main() {
    printf("Welcome to Snake Game Client\n");
    printf("1. Start Server\n");
    printf("2. Connect to existing server\n");

    char choice;
    scanf(" %c", &choice);

    if (choice == '1') {
        startServer();
    } else {
        printf("Connecting to the existing server...\n");
    }

    sleep(1);

    int fd = shm_open(SHM_NAME, O_RDWR,  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (fd < 0) {
        perror("shm_open");
        return 1;
    }

    sharedData = mmap(NULL, sizeof(*sharedData), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (sharedData == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }
    close(fd);

    printf("Connected to server.\n");

    pthread_t inputThread, outputThread;

    if (pthread_create(&inputThread, NULL, inputThreadFunc, NULL) != 0) {
        perror("pthread_create");
        munmap(sharedData, sizeof(*sharedData));
        return 1;
    }

    if (pthread_create(&outputThread, NULL, outputThreadFunc, NULL) != 0) {
        perror("pthread_create");
        munmap(sharedData, sizeof(*sharedData));
        return 1;
    }

    if (pthread_join(inputThread, NULL) != 0) {
        perror("pthread_join");
    }
    if (pthread_join(outputThread, NULL) != 0) {
        perror("pthread_join");
    }

    if (sem_wait(&sharedData->sem) != 0) {
        perror("sem_wait");
    } else {
        printf("\n%s", sharedData->game_over_reason);
        printf("Final score: %d\n", sharedData->score);
        if (sem_post(&sharedData->sem) != 0) {
            perror("sem_post");
        }
    }

    if (munmap(sharedData, sizeof(*sharedData)) != 0) {
        perror("munmap");
    }

    return 0;
}
