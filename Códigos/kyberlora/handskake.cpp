// handshake.cpp
#include "handshake.h"
#include "config.h"
#include "lora.h"
#include "display.h"
#include <Arduino.h>
#include <string.h>

bool handshake() {
#if MASTER == 1
  uint8_t ack[4];
  Serial.println("[M] Enviando HELLO...");
  oled_print(1, "Handshake..."); oled_show();
  while (1) {
    lora_send((uint8_t*)"HELLO", 5);
    delay(100);
    if (lora_recv(ack, 1000) == 3 && memcmp(ack, "ACK", 3) == 0) {
      Serial.println("[M] ACK recebido.");
      oled_print(2, "ACK OK"); oled_show(); delay(500);
      return true;
    }
  }
#else
  uint8_t buf[10];
  Serial.println("[S] Aguardando HELLO...");
  oled_print(1, "Aguardando HELLO"); oled_show();
  while (1) {
    if (lora_recv(buf, 5000) == 5 && memcmp(buf, "HELLO", 5) == 0) {
      delay(100);
      lora_send((uint8_t*)"ACK", 3);
      delay(100);
      lora_send((uint8_t*)"ACK", 3);
      Serial.println("[S] ACK enviado.");
      oled_print(2, "ACK enviado"); oled_show(); delay(500);
      return true;
    }
  }
#endif
}
