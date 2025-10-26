/* Bibliotecas para comunicação via protocolo LoRa */
#include <LoRa.h>
#include <SPI.h>

int num = 0, recebido = 0, cont = 0;

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

/* Programa principal */
void loop() 
{       
    Serial.print("Fibonnaci de ordem: ");
    Serial.println(cont);
    Serial.println(num);
    LoRa.beginPacket();
    LoRa.write((uint8_t*)&num, sizeof(num));
    LoRa.write((uint8_t*)&cont, sizeof(cont));
    LoRa.endPacket();

    uint8_t* ptr = (uint8_t*)&recebido;
    int tam = 0;
    while(tam == 0) {
        tam = LoRa.parsePacket();
    }

    while(LoRa.available() > sizeof(int)) {
        *ptr = LoRa.read();
        ptr++;
    }
    ptr = (uint8_t*)&cont;
    while(LoRa.available()) {
        *ptr = LoRa.read();
        ptr++;
    }
    cont++;
    num = num+recebido;
    delay(1500);
}