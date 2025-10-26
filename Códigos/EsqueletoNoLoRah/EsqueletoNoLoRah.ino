/* Bibliotecas para comunicação via protocolo LoRa */
#include <LoRa.h>
#include <SPI.h>

/* Bibliotecas para controle gráfico */
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/* Instância da Classe Adafruit para controle da Tela */
Adafruit_SSD1306 display(128, 64, &Wire, 16);

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
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Display inicializado!");
    display.display();
}

/* Programa principal */
void loop() 
{
    // vazio por enquanto
}
