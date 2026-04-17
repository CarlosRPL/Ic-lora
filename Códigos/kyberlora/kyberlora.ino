#include "config.h"
#include "display.h"
#include "lora.h"
#include "kyber.h"
#include "handshake.h"
#include <LoRa.h>
// Globais de aplicação
static PublicKey  pk;
static SecretKey  sk;
static Ciphertext ct;

void setup() {
  Serial.begin(115200);
  display_init();
  lora_init();
  oled_print(0, MASTER ? "MASTER" : "SLAVE"); oled_show();
  delay(2000);
  if (!handshake()) {
    oled_print(0, "Handshake falhou"); oled_show();
    while (1);
  }
}

void loop() {
#if MASTER == 1
  static bool sent_pk = false;
  if (!sent_pk) {
    oled_clear(); oled_print(0, "Gerando chaves..."); oled_show();
    kyber_keygen(pk, sk, 0x12345678, 0x9ABCDEF0);
    oled_print(1, "Enviando PK..."); oled_show();
    send_large((uint8_t*)&pk, sizeof(pk));
    oled_print(2, "Aguardando CT..."); oled_show();
    sent_pk = true;
  }
  LoRa.receive();
  while (LoRa.parsePacket()) LoRa.read();
  size_t ct_len;
  if (recv_large((uint8_t*)&ct, sizeof(ct), &ct_len, 60000) && ct_len == sizeof(ct)) {
    uint8_t bits[256], msg[32];
    kyber_decrypt(bits, 256, sk, ct);
    bits_para_bytes(msg, bits, 32);
    msg[31] = 0;
    Serial.printf("[M] Mensagem: %s\n", msg);
    oled_clear(); oled_print(0, "Mensagem:"); oled_print(1, (char*)msg); oled_show();
    lora_send((uint8_t*)"OK", 2);
    delay(10000); sent_pk = false;
  }
#else
  static bool received_pk = false;
  if (!received_pk) {
    oled_clear(); oled_print(0, "Aguardando PK..."); oled_show();
    LoRa.receive();
    size_t pk_len;
    if (recv_large((uint8_t*)&pk, sizeof(pk), &pk_len, 60000) && pk_len == sizeof(pk)) {
      received_pk = true;
      lora_send((uint8_t*)"OK", 2);
      const char *msg = "Ola Kyber!";
      uint8_t bits[256];
      bytes_para_bits(bits, (uint8_t*)msg, strlen(msg)+1);
      oled_print(1, "Cifrando..."); oled_show();
      kyber_encrypt(ct, pk, bits, 256, 0xDEADBEEF);
      oled_print(2, "Enviando CT..."); oled_show();
      delay(200);
      send_large((uint8_t*)&ct, sizeof(ct));
      uint8_t ack[3];
      lora_recv(ack, 5000);
      oled_print(3, "Pronto."); oled_show();
      delay(10000); received_pk = false;
    }
  }
#endif
}
