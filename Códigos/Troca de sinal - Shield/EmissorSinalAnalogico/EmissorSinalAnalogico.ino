/* Bibliotecas para comunicação via protocolo LoRa */
#include <LoRa.h>
#include <SPI.h>

/* Função de setup */
void setup() 
{
    /* Define a comunicação SPI-LoRa e inicia o protocolo LoRa na frequência desejada */
    LoRa.setPins(10, 9, 2);
    LoRa.begin(868E6);

    /* Ajusta os parâmetros da conexão LoRa */
    LoRa.setSpreadingFactor(12);
    LoRa.setTxPower(20);
    LoRa.setSignalBandwidth(125E3);

    Serial.begin(9600);

}
int valor = 0;
/* Programa principal */
void loop() 
{       
  valor = analogRead(1);
  LoRa.beginPacket();
  LoRa.write((uint8_t*) &valor, sizeof(int));
  LoRa.endPacket();
  delay(1000);
}