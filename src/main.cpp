/*
  OUTPUT: Creating your own hotspot on esp32 wifi+ble module.
  Author: Ankit Rana (Futechiot)
  Board Used: esp32 development board, LolinD32,WEMOS LOLIN32, ESP32 MH-ET live Minikit
  Website: www.futechiot.com
  GitHub: https://github.com/futechiot
  
*/

#include <WiFi.h> 
#include <Arduino.h>             //wifi library for ESp32 to access other functionalities
#include <ESP32Encoder.h>
#include "Pins.h"
#include <Motor.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>


// --- CONFIGURAÇÕES ---
const int PWM_TEST = 256; // PWM de teste (0-1023)
const int PULSES_PER_REV = 28;
const uint16_t SAMPLE_TIME = 100; // Amostragem em ms (influencia no cálculo do RPM e resolução)
const float BIAS_CORRECTION= 1.008840; // Fator calculado via Mínimos Quadrados
const uint16_t TIMEOUT = 5000; // Tempo em ms para timeout de comunicação

// --- Configurações de Rede ---
const char* ssid = "Tangas_Frouxas";
const char* password = "tangas321";

// --- VARIÁVEIS GLOBAIS ---
TimerHandle_t encoderTimer = NULL;
volatile int32_t lpulsesInWindow = 0; // Quantidade de pulsos contados por tempo de amostragem
volatile int32_t rpulsesInWindow = 0;
volatile bool calculateRPM = false;
int16_t currentPWM = 0;
int16_t current_rpm = 0;
uint16_t timeoutTimer = 0;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Crie um objeto encoder
ESP32Encoder lencoder;
ESP32Encoder rencoder;

// Motor A (LEFT)
Motor lmotor(AIN1, AIN2, PWMA);

// Motor B (RIGHT)
Motor rmotor(BIN1, BIN2, PWMB, 1);

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

// Protótipos de funções
void gatherEncoderData(TimerHandle_t xTimer);
void wait(int Time);
void handleJsonMessage(void *arg, uint8_t *data, size_t len, AsyncWebSocketClient *client);
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len);


void setup() {

  Serial.begin(115200);                                     

  // WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000); Serial.println("Conectando...");
  }
  Serial.println(WiFi.localIP());               


  encoderTimer = xTimerCreate(
    "EncoderTimer",                   // Timer name
    pdMS_TO_TICKS(SAMPLE_TIME),      // Período em ticks
    pdTRUE,                         // Auto-reload (periodic timer)
    NULL,                           // Timer ID
    gatherEncoderData                  // Callback function
  );

  if (encoderTimer == NULL) {
    Serial.println("Failed to create timer!");
    while (1);
  }

  rmotor.switchInput();
  lmotor.switchInput();

  // Habilita os resistores de pull-up internos do ESP32 (recomendado para encoders)
  ESP32Encoder::useInternalWeakPullResistors = puType::up;
  // Você também pode usar DOWN se o seu encoder precisar de pull-downs, ou NONE se já tiver resistores externos.

  // Anexa os pinos ao objeto encoder usando o modo de quadratura completa
  // Isso usa ambas as bordas de ambos os canais A e B para 4x a resolução
  lencoder.attachFullQuad(encoderAChannel1, encoderAChannel2);
  rencoder.attachFullQuad(encoderBChannel1, encoderBChannel2);

  lencoder.clearCount();
  rencoder.clearCount();

  xTimerStart(encoderTimer, 0);

  // Starts the WebSocket server
  ws.onEvent(onEvent);
  server.addHandler(&ws);
  server.begin();
}

void loop() {

  // Calculates RPM periodically by SAMPLE_TIME ms
  if (calculateRPM) {
    int32_t pulses = 0;
    bool doCalc = false;

    portENTER_CRITICAL(&mux);
    pulses = lpulsesInWindow;
    doCalc = calculateRPM;
    calculateRPM = false;
    portEXIT_CRITICAL(&mux);

    if (doCalc) {
      float rpm_raw = ((float)pulses / (float)PULSES_PER_REV) * (60000.0f/(float)SAMPLE_TIME);
      current_rpm = rpm_raw*BIAS_CORRECTION;
      Serial.printf(">RPM:%.2f\n", rpm_raw);
    }
  }
  
  // Stops the motors if no command received within TIMEOUT ms
  if (millis() - timeoutTimer > TIMEOUT) {
    currentPWM = 0;
    lmotor.setSpeed(currentPWM);
    rmotor.setSpeed(currentPWM);
  }
}

// --- Processamento do JSON ---
void handleJsonMessage(void *arg, uint8_t *data, size_t len, AsyncWebSocketClient *client) {
  // Creates a JSON buffer
  JsonDocument doc; 

  // 2. Tries to parse the JSON message
  DeserializationError error = deserializeJson(doc, data, len);

  if (error) {
    Serial.print(F("Falha no deserializeJson: "));
    Serial.println(error.f_str());
    return;
  }

  // 3. Verifica qual é o comando
  const char* command = doc["cmd"]; // "setPWM" ou "getRPM"

  // CASO 1: Escrever PWM
  if (strcmp(command, "setPWM") == 0) {
    if(doc["val"].is<int>()) {
      currentPWM = doc["val"];
      
      // Limita entre 0 e 255
      if(currentPWM > 1023) currentPWM = 1023;
      if(currentPWM < 0) currentPWM = 0;

      lmotor.setSpeed(currentPWM);
      rmotor.setSpeed(currentPWM);
      timeoutTimer = millis(); // Resets timeout timer
      Serial.printf("Comando recebido: PWM ajustado para %d\n", currentPWM);
    }
  }
  
  // CASO 2: Solicitar RPM
  else if (strcmp(command, "getRPM") == 0) {
    
    // Prepara a resposta JSON
    JsonDocument responseDoc;
    responseDoc["rpm"] = current_rpm;
    
    String responseText;
    serializeJson(responseDoc, responseText);
    
    // Envia APENAS para o cliente que solicitou (MATLAB)
    client->text(responseText);
    Serial.printf("RPM solicitado. Enviando: %d\n", current_rpm);
  }
}

// --- Eventos do WebSocket ---
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("Cliente conectado %u\n", client->id());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("Desconectado: %u\n", client->id());
      break;
    case WS_EVT_DATA:
      // Call handler for JSON message
      AwsFrameInfo *info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        handleJsonMessage(arg, data, len, client);
      }
      break;
  }
}

// ISR for timed gathering of encoder data
void gatherEncoderData(TimerHandle_t xTimer) {
  portENTER_CRITICAL(&mux);
  lpulsesInWindow = lencoder.getCount();
  rpulsesInWindow = rencoder.getCount();
  lencoder.clearCount();
  rencoder.clearCount();
  calculateRPM = true;
  portEXIT_CRITICAL(&mux);
}

// Delay function that doesn't engage sleep mode
void wait(int time) {
  int lasttime = millis();
  vTaskDelay(time / portTICK_PERIOD_MS);
}