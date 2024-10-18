/*
==================== Contador Regressivo ====================
Versão: 1.02
Autor: Sergio Augusto
Data: 2023-10-09
Licença: MIT License

Copyright (c) 2023 Sergio Augusto

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

==================== Funcionamento do Sistema ====================

Este sistema é um contador regressivo digital com as seguintes funcionalidades:

1. Configuração do tempo: Use o encoder rotativo para ajustar o tempo desejado.
   Gire lentamente para incrementos/decrementos de 1 segundo, ou rapidamente
   para incrementos/decrementos de 10 segundos.

2. Iniciar contagem: Pressione o botão do encoder para iniciar a contagem regressiva.

3. Pausar/Cancelar: Durante a contagem, pressione o botão para cancelar.

4. Display OLED: Mostra o tempo restante, status atual e uma barra de progresso.

5. Alarme sonoro: Ao final da contagem, um alarme sonoro é ativado.

6. Controle de relé: Um relé é ativado durante a contagem, permitindo controlar
   dispositivos externos.

7. Reinício: Após o término ou cancelamento, pressione o botão para reiniciar
   o sistema e configurar um novo tempo.

8. Salvamento da última configuração: O último tempo configurado é salvo na EEPROM
   e carregado na próxima inicialização.

O sistema utiliza um Arduino, um encoder rotativo para entrada, um display OLED
para visualização, um buzzer para alarme sonoro e um relé para controle externo.

==================== Diagrama de Conexão ====================

+---------------------------------------------+
|     Arduino Uno                             |
+---------------------------------------------+
|                                             |
| D2 -------- CLK (Encoder)                   |  
| D3 -------- DT  (Encoder)                   |
| D4 -------- SW  (Encoder)                   |
| D7 -------- Buzzer                          |
| D6 -------- Relé                            |
| A4 -------- SDA (Display OLED)              |
| A5 -------- SCL (Display OLED)              |
| GND ------- GND (Todos os componentes)      |
| 5V -------- VCC (Todos os componentes)      |
|                                             |
+---------------------------------------------+

Notas:
1. O encoder rotativo deve ter resistores pull-up nos pinos CLK e DT se não forem internos.
2. O relé deve ser conectado através de um transistor ou optoacoplador para proteção.
3. O display OLED I2C geralmente requer resistores pull-up nas linhas SDA e SCL.
4. Certifique-se de que todos os componentes estejam alimentados corretamente.

============================================================
*/

//==================== Inclusão de Bibliotecas ==================//
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <EEPROM.h>
//==================== Fim da Inclusão de Bibliotecas ==================//

//==================== Definições de Hardware ==================//
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
//==================== Fim das Definições de Hardware ==================//

//==================== Mapeamento de Hardware ==================//
#define ENCODER_CLK 2
#define ENCODER_DT 3
#define ENCODER_SW 4
#define BUZZER_PIN 7
#define RELAY_PIN 6
//==================== Fim do Mapeamento de Hardware ==================//

//==================== Objetos ==================//
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//==================== Fim dos Objetos ==================//

//==================== Variáveis Globais ==================//
volatile int encoderPos = 0;
int lastReportedPos = 0;
static boolean rotating = false;

int seconds = 0;
int minutes = 0;
int initialTotalSeconds = 0;
bool countdownStarted = false;
unsigned long lastCountdownUpdate = 0;
unsigned long lastButtonPress = 0;
unsigned long lastEncoderChange = 0;
const unsigned long DEBOUNCE_DELAY = 300;
const unsigned long FAST_THRESHOLD = 60; // Limiar para considerar rotação rápida (ms)

const int MIN_TIME = 1;
const int MAX_TIME = 1800;  // 30 min Tempo Maximo

// Endereço na EEPROM para salvar a configuração
const int EEPROM_ADDRESS = 0;
//==================== Fim das Variáveis Globais ==================//

//==================== Configuração Inicial ==================//
void setup() {
  // Configuração dos pinos
  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  pinMode(ENCODER_SW, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);

  // Configuração da interrupção do encoder
  attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), doEncoder, CHANGE);

  // Inicialização do display OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for(;;);  // Loop infinito se o display não inicializar
  }
  display.setTextColor(WHITE);
  
  // Carrega a última configuração salva
  loadLastConfiguration();
  
  // Exibição da tela inicial e reprodução do som de inicialização
  showSplashScreen();
  playStartupSound();
}
//==================== Fim da Configuração Inicial ==================//

//==================== Loop Principal ==================//
void loop() {
  rotating = true;

  // Verifica se a posição do encoder mudou
  if (lastReportedPos != encoderPos) {
    if (!countdownStarted) {
      // Calcula o incremento baseado na velocidade de rotação
      int increment = 1;
      unsigned long currentTime = millis();
      if (currentTime - lastEncoderChange < FAST_THRESHOLD) {
        increment = 10;
      }
      lastEncoderChange = currentTime;

      // Atualiza o tempo baseado na mudança do encoder
      int totalSeconds = minutes * 60 + seconds + (encoderPos - lastReportedPos) * increment;
      totalSeconds = constrain(totalSeconds, MIN_TIME, MAX_TIME);
      
      minutes = totalSeconds / 60;
      seconds = totalSeconds % 60;
      
      updateDisplay();
      saveLastConfiguration(); // Salva a nova configuração
    }
    lastReportedPos = encoderPos;
  }

  // Verifica se o botão foi pressionado
  if (isButtonPressed()) {
    if (!countdownStarted) {
      // Inicia a contagem regressiva
      countdownStarted = true;
      initialTotalSeconds = minutes * 60 + seconds;
      digitalWrite(RELAY_PIN, HIGH);  // Liga o relé
      lastCountdownUpdate = millis();
    } else {
      // Aborta a contagem regressiva
      abortCountdown();
    }
  }

  // Atualiza a contagem regressiva se estiver em andamento
  if (countdownStarted) {
    if (millis() - lastCountdownUpdate >= 1000) {
      lastCountdownUpdate = millis();
      if (seconds > 0 || minutes > 0) {
        if (seconds == 0) {
          minutes--;
          seconds = 59;
        } else {
          seconds--;
        }
        updateDisplay();
      } else {
        countdownFinished();
      }
    }
  }
}
//==================== Fim do Loop Principal ==================//

//==================== Funções Auxiliares ==================//
// Função de interrupção para o encoder
void doEncoder() {
  if (digitalRead(ENCODER_CLK) == digitalRead(ENCODER_DT)) {
    encoderPos++;
  } else {
    encoderPos--;
  }
}

// Atualiza o display OLED
void updateDisplay() {
  display.clearDisplay();
  display.setRotation(2); // Rotaciona a tela em 180 Graus
  
  // Exibe o status atual (Configurar ou Contagem)
  display.setFont(&FreeSerif9pt7b);
  int16_t x1, y1;
  uint16_t w, h;
  
  String statusText = countdownStarted ? "Contagem" : "Configurar";
  display.getTextBounds(statusText, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 12);
  display.println(statusText);
  
  // Exibe o tempo atual
  display.setFont(&FreeMonoBold12pt7b);
  char timeStr[6];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d", minutes, seconds);
  display.getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 35);
  display.print(timeStr);
  
  // Exibe a ação disponível (iniciar ou cancelar)
  display.setFont();
  String actionText = countdownStarted ? "Pressione: Cancelar" : "Pressione: Iniciar";
  display.getTextBounds(actionText, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 45);
  display.println(actionText);
  
  // Desenha a barra de progresso
  int totalSeconds = minutes * 60 + seconds;
  int progress;
  if (countdownStarted) {
    progress = map(totalSeconds, 0, initialTotalSeconds, 0, SCREEN_WIDTH);
  } else {
    progress = map(totalSeconds, 0, MAX_TIME, 0, SCREEN_WIDTH);
  }
  display.fillRect(0, 60, progress, 4, WHITE);
  
  display.display();
}

// Função chamada quando a contagem regressiva termina
void countdownFinished() {
  digitalWrite(RELAY_PIN, LOW);  // Desliga o relé
  display.clearDisplay();
  display.setRotation(2);  // Rotaciona a tela em 180 Graus
  
  display.setFont(&FreeSerif9pt7b);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds("Tempo acabou!", 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 35);
  display.println("Tempo Acabou!");
  
  display.setFont();
  display.getTextBounds("Pressione: Reiniciar", 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 45);
  display.println("Pressione: Reiniciar");
  display.display();
  
  playAlarm();
  
  while (!isButtonPressed()) {
    // Espera o botão ser pressionado para reiniciar
  }
  
  resetTimer();
}

// Função chamada quando a contagem regressiva é abortada
void abortCountdown() {
  digitalWrite(RELAY_PIN, LOW);  // Desliga o relé
  display.clearDisplay();
  display.setRotation(2);  // Rotaciona a tela em 180 Graus
  
  display.setFont(&FreeSerif9pt7b);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds("Cancelado!", 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 35);
  display.println("Cancelado!");
  
  display.setFont();
  display.getTextBounds("Pressione: Reiniciar", 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 45);
  display.println("Pressione: Reiniciar");
  display.display();
  
  playCancel();
 
  while (!isButtonPressed()) {
    // Espera o botão ser pressionado para reiniciar
  }
  
  resetTimer();
}

// Reinicia o timer
void resetTimer() {
  countdownStarted = false;
  encoderPos = lastReportedPos;
  updateDisplay();
}

// Exibe a tela inicial
void showSplashScreen() {
  display.clearDisplay();
  display.setRotation(2);  // Rotaciona a tela em 180 Graus
  
  // Título
  display.setFont(&FreeSerif9pt7b);
  int16_t x1, y1;
  uint16_t w, h;
  
  display.getTextBounds("Camara UV", 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 15);
  display.println("Camara UV");
  
  display.getTextBounds("para PCB", 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 35);
  display.println("para PCB");
  
  // Versão e autor
  display.setFont();
  
  // Versão centralizada
  display.getTextBounds("v1.02", 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, SCREEN_HEIGHT - 20);
  display.println("v1.02");
  
  // Nome do autor em uma linha
  display.getTextBounds("por Sergio Augusto", 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, SCREEN_HEIGHT - 10);
  display.print("por Sergio Augusto");
  
  // Desenha uma borda ao redor da tela
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
  
  display.display();
  delay(2000);
}

// Reproduz o som de inicialização
void playStartupSound() {
  int melody[] = {262, 330, 392, 523};
  int durations[] = {100, 100, 100, 300};
  
  for (int i = 0; i < 4; i++) {
    tone(BUZZER_PIN, melody[i], durations[i]);
    delay(durations[i] + 50);
  }
  noTone(BUZZER_PIN);
}

// Reproduz o som de fim de contagem
void playAlarm() {
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, 1000, 500);
    delay(600);
    tone(BUZZER_PIN, 800, 500);
    delay(600);
  }
  noTone(BUZZER_PIN);
}


// Reproduz o som de fim de contagem
void playCancel() {
  //for (int i = 0; i < 2; i++) {
    tone(BUZZER_PIN, 200, 500);
    delay(600);
    tone(BUZZER_PIN, 100, 500);
    delay(600);
 // }
  noTone(BUZZER_PIN);
}

// Verifica se o botão foi pressionado (com debounce)
bool isButtonPressed() {
  if (digitalRead(ENCODER_SW) == LOW) {
    if (millis() - lastButtonPress > DEBOUNCE_DELAY) {
      lastButtonPress = millis();
      return true;
    }
  }
  return false;
}

// Salva a última configuração na EEPROM
void saveLastConfiguration() {
  int totalSeconds = minutes * 60 + seconds;
  EEPROM.put(EEPROM_ADDRESS, totalSeconds);
}

// Carrega a última configuração da EEPROM
void loadLastConfiguration() {
  int totalSeconds;
  EEPROM.get(EEPROM_ADDRESS, totalSeconds);
  
  // Verifica se o valor lido é válido
  if (totalSeconds >= MIN_TIME && totalSeconds <= MAX_TIME) {
    minutes = totalSeconds / 60;
    seconds = totalSeconds % 60;
  } else {
    // Se o valor não for válido, define um tempo padrão
    minutes = 5;
    seconds = 0;
  }
}
//==================== Fim das Funções Auxiliares ==================//
