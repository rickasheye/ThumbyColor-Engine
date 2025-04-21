#include "thumbycolor_engine.h"
#include <math.h>
#include <algorithm>
#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    Serial.println("Starting display test...");
    
    // Initialize display
    ThumbyColor.begin();
    Serial.println("Display initialized");
    
    // Clear screen
    ThumbyColor.clearScreen(rgb565(0x000000));
    Serial.println("Screen cleared");
    
    // Draw a test pattern with larger squares
    for(int i = 0; i < DISPLAY_WIDTH; i += 64) {
        for(int j = 0; j < DISPLAY_HEIGHT; j += 64) {
            // Top-left: Red
            ThumbyColor.fillRect(i, j, 32, 32, rgb565(0xFF0000));
            // Top-right: Green
            ThumbyColor.fillRect(i+32, j, 32, 32, rgb565(0x00FF00));
            // Bottom-left: Blue
            ThumbyColor.fillRect(i, j+32, 32, 32, rgb565(0x0000FF));
            // Bottom-right: White
            ThumbyColor.fillRect(i+32, j+32, 32, 32, rgb565(0xFFFFFFF));
        }
    }
    ThumbyColor.update();
    Serial.println("Test pattern drawn");
}

void loop() {
    // Just keep the test pattern displayed
    delay(1000);
}