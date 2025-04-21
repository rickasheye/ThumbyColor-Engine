// thumbycolor_engine.h

#ifndef THUMBYCOLOR_ENGINE_H
#define THUMBYCOLOR_ENGINE_H

#include <stdint.h>
#include <stddef.h>

//Display Configuration
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 128
#define BYTES_PER_PIXEL 2
#define FB_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT * BYTES_PER_PIXEL)

// Pin Definitions
#define LCD_DC_PIN 16
#define LCD_CS_PIN 17
#define LCD_CLK_PIN 18
#define LCD_MOSI_PIN 19
#define LCD_RST_PIN 4
#define LCD_BL_PIN 7

// Button Definitions
#define BTN_UP 1
#define BTN_DOWN 3
#define BTN_LEFT 0
#define BTN_RIGHT 2
#define BTN_A 21
#define BTN_B 25

// Additional Controls
#define VIB_PIN 5
#define GPIO_PWM_LED_R 11
#define GPIO_PWM_LED_G 10
#define GPIO_PWM_LED_B 12
#define GPIO_BUTTON_BUMPER_LEFT 6
#define GPIO_BUTTON_BUMPER_RIGHT 22
#define GPIO_BUTTON_MENU 26

//need to reverse this to be RGB and not inverted.
uint16_t rgb565(unsigned long rgb);
uint16_t swapColorBytes(uint16_t color);

class ThumbyColorEngine {
public:
  ThumbyColorEngine();
  void begin();
  void update();

  // Display functions
  void clearScreen(uint16_t color = rgb565(0x000000));
  void drawPixel(int16_t x, int16_t y, uint16_t color);
  void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

  // Input functions
  bool buttonPressed(uint8_t button);
  bool buttonReleased(uint8_t button);
  bool buttonHeld(uint8_t button);

  // LED and feedback functions
  void setRGB(uint8_t r, uint8_t g, uint8_t b);
  void setVibration(uint8_t strength);

  // Frame buffer access
  uint16_t* getFrameBuffer() {
    return frameBuffer;
  }
  void display();  // Push frame buffer to screen

private:
  void setupPins();
  void setupPWM();
  void setupDisplay();
  void sendCommand(uint8_t cmd, const uint8_t* data = nullptr, size_t length = 0);
  void setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

  static uint16_t frameBuffer[DISPLAY_WIDTH * DISPLAY_HEIGHT];
  uint8_t buttonStates;
  uint8_t prevButtonStates;
};

extern ThumbyColorEngine ThumbyColor;

#endif  // THUMBYCOLOR_ENGINE_H
