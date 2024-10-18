# UVPCB
"Arduino-based UV exposure timer for PCB production with OLED display and rotary encoder control"


# Câmara UV para PCB com Timer Digital

Este projeto implementa um sistema de controle para uma câmara de exposição UV utilizada na produção de placas de circuito impresso (PCBs). O sistema utiliza um Arduino para controlar o tempo de exposição, com interface através de um encoder rotativo e display OLED.

## Características

- Controle preciso do tempo de exposição UV
- Interface de usuário com display OLED
- Entrada via encoder rotativo
- Alarme sonoro ao final da exposição
- Controle de relé para ativar/desativar a lâmpada UV
- Salvamento da última configuração na EEPROM

## Hardware Necessário

- Arduino Uno ou compatível
- Display OLED 128x64 (I2C)
- Encoder rotativo com botão
- Buzzer
- Relé
- Lâmpada UV (não incluída no código)

## Instalação

1. Clone este repositório ou faça o download do código.
2. Abra o arquivo `UVPCB.ino` no Arduino IDE.
3. Instale as bibliotecas necessárias (Adafruit_GFX, Adafruit_SSD1306, Wire.h, Fonts/FreeSerif9pt7b, Fonts/FreeMonoBold12pt7b, EEPROM).
4. Conecte o hardware conforme descrito no diagrama de conexão no código.
5. Faça o upload do código para o seu Arduino.

## Uso

1. Ligue o dispositivo.
2. Use o encoder rotativo para ajustar o tempo desejado.
3. Pressione o botão do encoder para iniciar a contagem regressiva.
4. O relé será ativado, ligando a lâmpada UV.
5. Ao final da contagem, um alarme sonoro será ativado e o relé desligará a lâmpada.

## Contribuições

Contribuições são bem-vindas! Sinta-se à vontade para abrir issues ou enviar pull requests com melhorias.

## Licença

Este projeto está licenciado sob a Licença MIT - veja o arquivo [LICENSE](LICENSE) para detalhes.
