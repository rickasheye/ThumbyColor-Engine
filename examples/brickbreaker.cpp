#include "thumbycolor_engine.h"
#include <Arduino.h>
#include <vector>
#include <algorithm>

struct Ball {
    float x, y;
    float dx, dy;
    int size = 4;
};

struct Paddle {
    int x, y;
    int width = 32;
    int height = 4;
};

struct Brick {
    int x, y;
    bool destroyed = false;
};

#define BRICK_ROWS 4
#define BRICK_COLS 8
#define BRICK_WIDTH 14
#define BRICK_HEIGHT 6
#define BRICK_GAP 2

Ball ball;
Paddle paddle;
std::vector<Brick> bricks;
bool gameOver = false;

void resetGame() {
    ball = { DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, 1.5f, -1.5f, 4 };
    paddle = { (DISPLAY_WIDTH - 32) / 2, DISPLAY_HEIGHT - 10 };

    bricks.clear();
    for (int r = 0; r < BRICK_ROWS; ++r) {
        for (int c = 0; c < BRICK_COLS; ++c) {
            int x = c * (BRICK_WIDTH + BRICK_GAP) + 4;
            int y = r * (BRICK_HEIGHT + BRICK_GAP) + 10;
            bricks.push_back({x, y});
        }
    }
    gameOver = false;
}

void drawGame() {
    ThumbyColor.clearScreen(rgb565(0x000000));

    // Draw ball
    ThumbyColor.fillRect((int)ball.x, (int)ball.y, ball.size, ball.size, rgb565(0x00FFFF));

    // Draw paddle
    ThumbyColor.fillRect(paddle.x, paddle.y, paddle.width, paddle.height, rgb565(0xFFFFFF));

    // Draw bricks
    for (auto& brick : bricks) {
        if (!brick.destroyed) {
            ThumbyColor.fillRect(brick.x, brick.y, BRICK_WIDTH, BRICK_HEIGHT, rgb565(0xFF6600));
        }
    }

    ThumbyColor.update();
}

void updateGame() {
    // Move paddle
    if (ThumbyColor.buttonHeld(BTN_LEFT)) {
        paddle.x -= 3;
        if (paddle.x < 0) paddle.x = 0;
    } else if (ThumbyColor.buttonHeld(BTN_RIGHT)) {
        paddle.x += 3;
        if (paddle.x + paddle.width > DISPLAY_WIDTH) paddle.x = DISPLAY_WIDTH - paddle.width;
    }

    // Move ball
    ball.x += ball.dx;
    ball.y += ball.dy;

    // Wall collision
    if (ball.x <= 0 || ball.x + ball.size >= DISPLAY_WIDTH) ball.dx *= -1;
    if (ball.y <= 0) ball.dy *= -1;

    // Bottom collision - game over
    if (ball.y + ball.size >= DISPLAY_HEIGHT) {
        gameOver = true;
        return;
    }

    // Paddle collision
    if (ball.y + ball.size >= paddle.y &&
        ball.x + ball.size >= paddle.x &&
        ball.x <= paddle.x + paddle.width) {
        ball.dy *= -1;
        ball.y = paddle.y - ball.size;
    }

    // Brick collision
    for (auto& brick : bricks) {
        if (brick.destroyed) continue;
        if (ball.x < brick.x + BRICK_WIDTH &&
            ball.x + ball.size > brick.x &&
            ball.y < brick.y + BRICK_HEIGHT &&
            ball.y + ball.size > brick.y) {
            brick.destroyed = true;
            ball.dy *= -1;
            break;
        }
    }

    // Check win condition
    bool allDestroyed = true;
    for (auto& b : bricks) {
        if (!b.destroyed) {
            allDestroyed = false;
            break;
        }
    }
    if (allDestroyed) {
        gameOver = true;
    }
}

void showGameOver(bool win) {
    ThumbyColor.clearScreen(rgb565(0x000000));

    uint16_t textColor = win ? rgb565(0x00FF00) : rgb565(0xFF0000);
    uint16_t barColor = win ? rgb565(0x003300) : rgb565(0x330000);

    ThumbyColor.fillRect(16, 48, 96, 32, barColor);
    ThumbyColor.fillRect(20, 56, 88, 4, textColor); // Fake "YOU WIN" or "GAME OVER"
    ThumbyColor.fillRect(20, 66, 88, 4, textColor);

    ThumbyColor.update();

    // Wait for A to restart
    while (!ThumbyColor.buttonPressed(BTN_A)) {
        delay(10);
    }
    while (ThumbyColor.buttonPressed(BTN_A)) {
        delay(10);
    }

    resetGame();
}

void setup() {
    Serial.begin(115200);
    ThumbyColor.begin();
    resetGame();
}

void loop() {
    if (!gameOver) {
        updateGame();
        drawGame();
    } else {
        showGameOver(bricks.end() == std::find_if(bricks.begin(), bricks.end(), [](Brick& b){ return !b.destroyed; }));
    }

    delay(16); // ~60 FPS
}
