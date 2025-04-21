#include "thumbycolor_engine.h"
#include <SPI.h>
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"

// Display Configuration
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 128
#define BYTES_PER_PIXEL 2
#define FB_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT * BYTES_PER_PIXEL)

// Initialize static member
uint16_t ThumbyColorEngine::frameBuffer[DISPLAY_WIDTH * DISPLAY_HEIGHT];

// Global instance
ThumbyColorEngine ThumbyColor;

ThumbyColorEngine::ThumbyColorEngine()
  : buttonStates(0), prevButtonStates(0) {
  memset(frameBuffer, 0, FB_SIZE);
}

void ThumbyColorEngine::begin() {
  Serial.begin(115200);
  Serial.println("ThumbyColor Engine Init");

  setupPins();
  setupPWM();
  setupDisplay();

  // Clear screen and set default LED state
  clearScreen();
  setRGB(128, 128, 128);
}

void ThumbyColorEngine::setupPins() {
  spi_init(spi0, 10000000);

  gpio_set_function(LCD_MOSI_PIN, GPIO_FUNC_SPI);
  gpio_set_function(LCD_CLK_PIN, GPIO_FUNC_SPI);

  gpio_init(LCD_CS_PIN);
  gpio_init(LCD_DC_PIN);
  gpio_init(LCD_RST_PIN);
  gpio_init(LCD_BL_PIN);

  gpio_set_dir(LCD_CS_PIN, GPIO_OUT);
  gpio_set_dir(LCD_DC_PIN, GPIO_OUT);
  gpio_set_dir(LCD_RST_PIN, GPIO_OUT);
  gpio_set_dir(LCD_BL_PIN, GPIO_OUT);

  // Initialize buttons
  const uint8_t buttons[] = {
    BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT,
    BTN_A, BTN_B,
    GPIO_BUTTON_BUMPER_LEFT, GPIO_BUTTON_BUMPER_RIGHT,
    GPIO_BUTTON_MENU
  };

  for (int i = 0; i < sizeof(buttons); i++) {
    gpio_init(buttons[i]);
    gpio_set_dir(buttons[i], GPIO_IN);
    gpio_pull_up(buttons[i]);
  }

  // Setup vibration
  gpio_init(VIB_PIN);
  gpio_set_dir(VIB_PIN, GPIO_OUT);
  gpio_put(VIB_PIN, 0);

  // Setup RGB LED pins
  gpio_set_function(GPIO_PWM_LED_R, GPIO_FUNC_PWM);
  gpio_set_function(GPIO_PWM_LED_G, GPIO_FUNC_PWM);
  gpio_set_function(GPIO_PWM_LED_B, GPIO_FUNC_PWM);
}

void ThumbyColorEngine::setupPWM() {
  uint slice_r = pwm_gpio_to_slice_num(GPIO_PWM_LED_R);
  uint slice_g = pwm_gpio_to_slice_num(GPIO_PWM_LED_G);
  uint slice_b = pwm_gpio_to_slice_num(GPIO_PWM_LED_B);

  pwm_set_wrap(slice_r, 255);
  pwm_set_wrap(slice_g, 255);
  pwm_set_wrap(slice_b, 255);

  pwm_set_enabled(slice_r, true);
  pwm_set_enabled(slice_g, true);
  pwm_set_enabled(slice_b, true);
}

void ThumbyColorEngine::setupDisplay() {
  Serial.println("Starting display setup...");

  // Initialize pins
  gpio_init(LCD_RST_PIN);
  gpio_init(LCD_CS_PIN);
  gpio_init(LCD_DC_PIN);
  gpio_init(LCD_BL_PIN);

  gpio_set_dir(LCD_RST_PIN, GPIO_OUT);
  gpio_set_dir(LCD_CS_PIN, GPIO_OUT);
  gpio_set_dir(LCD_DC_PIN, GPIO_OUT);
  gpio_set_dir(LCD_BL_PIN, GPIO_OUT);

  // Power up sequence
  gpio_put(LCD_CS_PIN, 1);
  gpio_put(LCD_DC_PIN, 1);
  gpio_put(LCD_BL_PIN, 0);

  // Initialize SPI
  Serial.println("Initializing SPI...");
  spi_init(spi0, 10000000);  // Start at 10MHz

  gpio_set_function(LCD_MOSI_PIN, GPIO_FUNC_SPI);
  gpio_set_function(LCD_CLK_PIN, GPIO_FUNC_SPI);

  // Hardware Reset
  Serial.println("Performing hardware reset...");
  gpio_put(LCD_CS_PIN, 1);
  gpio_put(LCD_RST_PIN, 1);
  delay(5);
  gpio_put(LCD_RST_PIN, 0);
  delay(10);
  gpio_put(LCD_RST_PIN, 1);
  delay(120);

  // Initialize Display
  Serial.println("Sending initialization commands...");
  sendCommand(0x01);  // Software Reset
  delay(150);

  sendCommand(0x11);  // Sleep Out
  delay(500);

  // Memory Data Access Control
  // Bit 7 - MY  (Row Address Order)
  // Bit 6 - MX  (Column Address Order)
  // Bit 5 - MV  (Row/Column Exchange)
  // Bit 4 - ML  (Vertical Refresh Order)
  // Bit 3 - RGB/BGR Order
  // Bit 2 - MH  (Horizontal Refresh Order)
  // Bit 1 - 0
  // Bit 0 - 0
  sendCommand(0x36, (uint8_t[]){ 0x48 }, 1);  // BGR color order, normal orientation

  // Interface Pixel Format
  sendCommand(0x3A, (uint8_t[]){ 0x55 }, 1);  // 16-bit color (RGB565)

  // Display Inversion Control
  sendCommand(0xB4, (uint8_t[]){ 0x00 }, 1);

  // Display ON
  sendCommand(0x29);
  delay(100);

  // Configure SPI for high speed
  Serial.println("Configuring SPI for high speed...");
  spi_set_format(spi0, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
  spi_init(spi0, 40000000);  // 40MHz

  // Turn on backlight
  Serial.println("Turning on backlight...");
  gpio_put(LCD_BL_PIN, 1);

  Serial.println("Display setup complete");
}

// Function to swap bytes in a 16-bit color value
uint16_t swapColorBytes(uint16_t color) {
    return (color << 8) | (color >> 8);
}

uint16_t rgb565(unsigned long rgb) {
  uint16_t R = ((rgb >> 16) & 0xFF);
  uint16_t G = ((rgb >> 8) & 0xFF);
  uint16_t B = ((rgb)&0xFF);

  uint16_t ret = (B & 0xF8) << 8;  // 5 bits
  ret |= (G & 0xFC) << 3;          // 6 bits
  ret |= (R & 0xF8) >> 3;          // 5 bits

  //invert the colors for some reason
  return ~swapColorBytes(ret);
}


void ThumbyColorEngine::sendCommand(uint8_t cmd, const uint8_t* data, size_t length) {
  gpio_put(LCD_CS_PIN, 0);
  gpio_put(LCD_DC_PIN, 0);
  spi_write_blocking(spi0, &cmd, 1);

  if (data && length > 0) {
    gpio_put(LCD_DC_PIN, 1);
    spi_write_blocking(spi0, data, length);
  }

  gpio_put(LCD_CS_PIN, 1);
}

void ThumbyColorEngine::setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  // Ensure coordinates are within bounds
  x0 = min(x0, (uint16_t)(DISPLAY_WIDTH - 1));
  y0 = min(y0, (uint16_t)(DISPLAY_HEIGHT - 1));
  x1 = min(x1, (uint16_t)(DISPLAY_WIDTH - 1));
  y1 = min(y1, (uint16_t)(DISPLAY_HEIGHT - 1));

  uint8_t data[4];

  data[0] = x0 >> 8;
  data[1] = x0 & 0xFF;
  data[2] = x1 >> 8;
  data[3] = x1 & 0xFF;
  sendCommand(0x2A, data, 4);

  data[0] = y0 >> 8;
  data[1] = y0 & 0xFF;
  data[2] = y1 >> 8;
  data[3] = y1 & 0xFF;
  sendCommand(0x2B, data, 4);

  sendCommand(0x2C);
}

void ThumbyColorEngine::setRGB(uint8_t r, uint8_t g, uint8_t b) {
  pwm_set_gpio_level(GPIO_PWM_LED_R, r);
  pwm_set_gpio_level(GPIO_PWM_LED_G, g);
  pwm_set_gpio_level(GPIO_PWM_LED_B, b);
}

void ThumbyColorEngine::setVibration(uint8_t strength) {
  gpio_put(VIB_PIN, strength > 0);
}

void ThumbyColorEngine::clearScreen(uint16_t color) {
  for (int i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT; i++) {
    frameBuffer[i] = color;
  }
}

void ThumbyColorEngine::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if (x >= 0 && x < DISPLAY_WIDTH && y >= 0 && y < DISPLAY_HEIGHT) {
    frameBuffer[y * DISPLAY_WIDTH + x] = color;
  }
}

void ThumbyColorEngine::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  for (int16_t i = x; i < x + w; i++) {
    drawPixel(i, y, color);
    drawPixel(i, y + h - 1, color);
  }
  for (int16_t i = y; i < y + h; i++) {
    drawPixel(x, i, color);
    drawPixel(x + w - 1, i, color);
  }
}

void ThumbyColorEngine::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  // Clip to display bounds
  if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT) return;
  if (x + w <= 0 || y + h <= 0) return;

  if (x < 0) {
    w += x;
    x = 0;
  }
  if (y < 0) {
    h += y;
    y = 0;
  }
  if (x + w > DISPLAY_WIDTH) w = DISPLAY_WIDTH - x;
  if (y + h > DISPLAY_HEIGHT) h = DISPLAY_HEIGHT - y;

  for (int16_t j = y; j < y + h; j++) {
    for (int16_t i = x; i < x + w; i++) {
      frameBuffer[j * DISPLAY_WIDTH + i] = color;
    }
  }
}

bool ThumbyColorEngine::buttonPressed(uint8_t button) {
  return !gpio_get(button);
}

bool ThumbyColorEngine::buttonReleased(uint8_t button) {
  return gpio_get(button);
}

bool ThumbyColorEngine::buttonHeld(uint8_t button){
  return (digitalRead(button) == LOW);
}

void ThumbyColorEngine::display() {
  setWindow(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);
  gpio_put(LCD_DC_PIN, 1);
  gpio_put(LCD_CS_PIN, 0);
  spi_write_blocking(spi0, (uint8_t*)frameBuffer, FB_SIZE);
  gpio_put(LCD_CS_PIN, 1);
}

void ThumbyColorEngine::update() {
  // Update button states
  prevButtonStates = buttonStates;
  buttonStates = 0;

  // Display the current frame buffer
  display();
}