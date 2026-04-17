#include "lora.h"
#include "config.h"
#include <LoRa.h>
#include <SPI.h>
#include <Arduino.h>

static inline size_t min_size(size_t a, size_t b) { return a < b ? a : b; }

void lora_init() {
  SPI.begin(5, 19, 27, 18);
  LoRa.setPins(SS_PIN, RST_PIN, DIO0_PIN);
  if (!LoRa.begin(FREQ)) {
    Serial.println("LoRa falhou!");
    while (1);
  }
  LoRa.setSpreadingFactor(SF);
  LoRa.setTxPower(TX_POWER);
  LoRa.setSignalBandwidth(BANDWIDTH);
  LoRa.receive();
  Serial.println("LoRa OK.");
}

void lora_send(const uint8_t *data, int len) {
  LoRa.beginPacket();
  LoRa.write(data, len);
  LoRa.endPacket();
}

int lora_recv(uint8_t *buf, uint32_t timeout_ms) {
  uint32_t start = millis();
  while (millis() - start < timeout_ms) {
    int sz = LoRa.parsePacket();
    if (sz) {
      int i = 0;
      while (LoRa.available() && i < sz) buf[i++] = LoRa.read();
      return i;
    }
    delay(5);
  }
  return 0;
}

bool send_large(const uint8_t *data, size_t total_len) {
  uint8_t num_blocks = (uint8_t)((total_len + BLOCK_SIZE - 1) / BLOCK_SIZE);
  Serial.printf("[TX] %u bytes | %u blocos\n", (unsigned)total_len, (unsigned)num_blocks);

  // Header com handshake
  bool hdr_acked = false;
  for (int r = 0; r < MAX_RETRIES && !hdr_acked; r++) {
    uint8_t hdr[4] = {
      MAGIC_HEADER,
      (uint8_t)(total_len & 0xFF),
      (uint8_t)((total_len >> 8) & 0xFF),
      num_blocks
    };
    LoRa.beginPacket(); LoRa.write(hdr, 4); LoRa.endPacket();
    uint8_t ack[2];
    if (lora_recv(ack, ACK_TIMEOUT) == 2 && ack[0] == 'H' && ack[1] == 'K') {
      hdr_acked = true;
    } else delay(150);
  }
  if (!hdr_acked) { Serial.println("[TX] ERRO: header sem ACK."); return false; }

  // Blocos com ACK individual
  for (uint8_t b = 0; b < num_blocks; b++) {
    size_t offset = (size_t)b * BLOCK_SIZE;
    size_t chunk  = min_size(BLOCK_SIZE, total_len - offset);
    bool   acked  = false;
    for (int r = 0; r < MAX_RETRIES && !acked; r++) {
      LoRa.beginPacket();
      LoRa.write(b);
      LoRa.write(&data[offset], chunk);
      LoRa.endPacket();
      uint8_t ack[2];
      if (lora_recv(ack, ACK_TIMEOUT) == 2 && ack[0] == 'B' && ack[1] == b)
        acked = true;
      else delay(100);
    }
    if (!acked) { Serial.printf("[TX] ERRO: bloco %u sem ACK!\n", b); return false; }
    delay(30);
  }
  return true;
}

bool recv_large(uint8_t *out_buf, size_t expected_max,
                size_t *out_len, uint32_t timeout_ms) {
  uint32_t start    = millis();
  bool     hdr_ok   = false;
  size_t   total_len = 0, received = 0;
  uint8_t  num_blks = 0;
  uint8_t *temp     = nullptr;
  bool     blocks_rcvd[MAX_BLOCKS] = {};

  while (millis() - start < timeout_ms) {
    int pkt = LoRa.parsePacket();
    if (!pkt) { delay(5); continue; }

    if (!hdr_ok) {
      if (pkt < 4) { while (LoRa.available()) LoRa.read(); continue; }
      uint8_t magic = LoRa.read();
      if (magic != MAGIC_HEADER) { while (LoRa.available()) LoRa.read(); continue; }
      uint8_t h[3]; LoRa.readBytes(h, 3);
      total_len = (size_t)(h[0] | (h[1] << 8));
      num_blks  = h[2];
      if (total_len > expected_max) { while (LoRa.available()) LoRa.read(); continue; }
      temp = (uint8_t*)malloc(total_len);
      if (!temp) { Serial.println("[RX] malloc falhou!"); return false; }
      memset(blocks_rcvd, 0, sizeof(blocks_rcvd));
      hdr_ok = true; received = 0;
      uint8_t ack[2] = {'H', 'K'};
      lora_send(ack, 2);
      continue;
    }

    if (pkt < 2) { while (LoRa.available()) LoRa.read(); continue; }
    uint8_t num = LoRa.read();
    size_t  len = (size_t)(pkt - 1);
    size_t  off = (size_t)num * BLOCK_SIZE;

    if (num >= MAX_BLOCKS || off + len > total_len) {
      while (LoRa.available()) LoRa.read();
    } else if (!blocks_rcvd[num]) {
      LoRa.readBytes(&temp[off], len);
      blocks_rcvd[num] = true;
      received += len;
      Serial.printf("[RX] Bloco %02u/%02u | %u bytes\n",
                    (unsigned)(num+1), (unsigned)num_blks, (unsigned)len);
    } else {
      while (LoRa.available()) LoRa.read();
    }

    uint8_t ack[2] = {'B', num};
    lora_send(ack, 2);

    if (received >= total_len) {
      memcpy(out_buf, temp, total_len);
      *out_len = total_len;
      free(temp);
      return true;
    }
  }
  if (temp) free(temp);
  return false;
}
