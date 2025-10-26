/* Bibliotecas para comunicação via protocolo LoRa */
#include <LoRa.h>
#include <SPI.h>

/* Bibliotecas para controle gráfico */
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/* Instância da Classe Adafruit para controle da Tela */
Adafruit_SSD1306 display(128, 64, &Wire, 16);

int num = 0, recebido = 0, cont = 0;

/* Função de setup */
void setup() 
{
    /* Define a comunicação SPI-LoRa e inicia o protocolo LoRa na frequência desejada */
    SPI.begin(5, 19, 27, 18);
    LoRa.setPins(18, 14, 26);
    LoRa.begin(868E6);

    /* Ajusta os parâmetros da conexão LoRa */
    LoRa.setSpreadingFactor(12);
    LoRa.setTxPower(20);
    LoRa.setSignalBandwidth(125E3);

    /* Define os parâmetros para controle gráfico da tela */
    Wire.begin(4, 15);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("Falha ao inicializar display OLED");
        for(;;); // trava se falhar
    }

    /* Configurações iniciais do display */
    display.clearDisplay();
    display.setTextSize(1.5);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Display inicializado!");
    display.display();
    delay(2000);
}

/* Programa principal */
void loop() 
{       
    display.clearDisplay();
    display.setCursor(0, 7);
    display.print("Fibonnaci de ordem:");
    display.print(cont);
    display.drawLine(0, 16, 128, 16, WHITE);
    display.setCursor(0, 28);
    display.print("-> ");
    display.println(num);
    display.drawLine(0, 48, 128, 48, WHITE);
    display.display();

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