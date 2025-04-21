#include "thumbycolor_engine.h"
#include <Arduino.h>
#include <deque>

#define SNAKE_SIZE 4
#define CELL_SIZE 8
#define MAX_LENGTH 64

struct Point {
    int x;
    int y;
};

std::deque<Point> snake;
Point food;
int dx = 1, dy = 0;
unsigned long lastMove = 0;
const unsigned long moveInterval = 200;
bool gameOver = false;

void spawnFood() {
    food.x = (rand() % (DISPLAY_WIDTH / CELL_SIZE)) * CELL_SIZE;
    food.y = (rand() % (DISPLAY_HEIGHT / CELL_SIZE)) * CELL_SIZE;
}

void resetGame() {
    snake.clear();
    for (int i = 0; i < SNAKE_SIZE; i++) {
        snake.push_front({CELL_SIZE * (5 - i), CELL_SIZE * 5});
    }
    dx = CELL_SIZE;
    dy = 0;
    spawnFood();
    gameOver = false;
}

void setup() {
    Serial.begin(115200);
    ThumbyColor.begin();
    ThumbyColor.clearScreen(rgb565(0x000000));
    resetGame();
}


void loop() {
    if (gameOver) {
        resetGame();
        return;
    }

    if (ThumbyColor.buttonPressed(BTN_DOWN) && dy == 0) {
        dx = 0; dy = -CELL_SIZE;
    } else if (ThumbyColor.buttonPressed(BTN_UP) && dy == 0) {
        dx = 0; dy = CELL_SIZE;
    } else if (ThumbyColor.buttonPressed(BTN_LEFT) && dx == 0) {
        dx = -CELL_SIZE; dy = 0;
    } else if (ThumbyColor.buttonPressed(BTN_RIGHT) && dx == 0) {
        dx = CELL_SIZE; dy = 0;
    }

    if (millis() - lastMove > moveInterval) {
        lastMove = millis();

        Point head = snake.front();
        Point newHead = { head.x + dx, head.y + dy };

        // Check wall collision
        if (newHead.x < 0 || newHead.y < 0 ||
            newHead.x >= DISPLAY_WIDTH || newHead.y >= DISPLAY_HEIGHT) {
            gameOver = true;
            return;
        }

        // Check self collision
        for (auto& part : snake) {
            if (part.x == newHead.x && part.y == newHead.y) {
                gameOver = true;
                return;
            }
        }

        snake.push_front(newHead);

        // Check food collision
        if (newHead.x == food.x && newHead.y == food.y) {
            spawnFood();
        } else {
            snake.pop_back();
        }

        ThumbyColor.clearScreen(rgb565(0x000000));

        // Draw food
        ThumbyColor.fillRect(food.x, food.y, CELL_SIZE, CELL_SIZE, rgb565(0x00FF00));

        // Draw snake
        for (auto& segment : snake) {
            ThumbyColor.fillRect(segment.x, segment.y, CELL_SIZE, CELL_SIZE, rgb565(0xFFFFFF));
        }

        ThumbyColor.update();
    }

    delay(10);
}
