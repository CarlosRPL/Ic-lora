#pragma once
#include <stdint.h>
#include <stddef.h>
#include <LoRa.h>
#define BLOCK_SIZE   200
#define MAX_BLOCKS    20
#define MAGIC_HEADER 0x7B
#define ACK_TIMEOUT  2000
#define MAX_RETRIES     5

void lora_init();
void lora_send(const uint8_t *data, int len);
int  lora_recv(uint8_t *buf, uint32_t timeout_ms);
bool send_large(const uint8_t *data, size_t total_len);
bool recv_large(uint8_t *out_buf, size_t expected_max,
                size_t *out_len, uint32_t timeout_ms = 60000);
