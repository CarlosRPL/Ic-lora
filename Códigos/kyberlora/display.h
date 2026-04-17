#pragma once
#include <Adafruit_SSD1306.h>

extern Adafruit_SSD1306 display;

void display_init();
void oled_clear();
void oled_show();
void oled_print(int row, const char *txt);
