#include "display.h"
#include "config.h"
#include <Wire.h>

Adafruit_SSD1306 display(128, 64, &Wire, -1);

void display_init() {
  Wire.begin(SDA_PIN, SCL_PIN);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("Display falhou!");
    while (1);
  }
  oled_clear(); oled_show();
}

void oled_clear() { display.clearDisplay(); }
void oled_show()  { display.display(); }

void oled_print(int row, const char *txt) {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, row * 12);
  display.println(txt);
}
