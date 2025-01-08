#include <esp_now.h>
#include <WiFi.h>
#include <Arduino.h>
#include <math.h>

uint8_t ControllerAddress[] = {0xFC, 0xB4, 0x67, 0x77, 0x9B, 0x5C};
uint8_t BoatAddress[] =       {0xE4, 0x65, 0xB8, 0x49, 0xC3, 0x68};

#define VRX_PIN  39
#define VRY_PIN  36
#define RESOLUTION 12
#define TESTING_ACTIVE true

// Structure for Data Transmission
// Left and right represent the left or right joystick on the controller
typedef struct struct_data {
  char  message[32];
  float leftJoystickX;
  float leftJoystickY;
  bool  leftSwitch;
  float rightJoystickX;
  float rightJoystickY;
  bool  rightSwitch;
} struct_data;

struct_data myData;

typedef struct struct_values {
  uint16_t ValueX = 0;
  uint16_t ValueY = 0;
  float    CorrectedValueX = 0.0;
  float    CorrectedValueY = 0.0;
} struct_values;

struct_values leftValues;
struct_values rightValues;


void onSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (TESTING_ACTIVE) {
    Serial.print("Nachricht gesendet: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Erfolg" : "Fehlgeschlagen");
  }
}

/**
 * Function that remaps the Joystick Values before they are sent via ESP-NOW
 */
void measureJoystickValue() {
  leftValues.ValueX = analogRead(VRX_PIN);   // try digitalread
  leftValues.ValueY = analogRead(VRY_PIN);
  leftValues.CorrectedValueX = map(leftValues.ValueX, 0, 4095, -208, 255);
  leftValues.CorrectedValueY = map(leftValues.ValueY, 0, 4095, 255, -310.5);
}

void setup() {
  if (TESTING_ACTIVE){
    Serial.begin(115200);
  }
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK && TESTING_ACTIVE) {
    Serial.println("ESP-NOW konnte nicht initialisiert werden");
    return;
  }

  esp_now_register_send_cb(onSent);

  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, BoatAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (!esp_now_is_peer_exist(BoatAddress)) {
    if (esp_now_add_peer(&peerInfo) != ESP_OK && TESTING_ACTIVE) {
      Serial.println("Empfänger konnte nicht hinzugefügt werden");
      return;
    }
  }

  strncpy(myData.message, "Daten werden versendet.", sizeof(myData.message) - 1);
  myData.message[sizeof(myData.message) - 1] = '\0';
}

void loop() {
  measureJoystickValue();
  myData.leftJoystickX = leftValues.CorrectedValueX;
  myData.leftJoystickY = leftValues.CorrectedValueY;

  esp_err_t result = esp_now_send(BoatAddress, (uint8_t *) &myData, sizeof(myData));

  if (result == ESP_OK && TESTING_ACTIVE) {
    Serial.println("Daten erfolgreich gesendet");
    Serial.print("Joystick X: ");
    Serial.println(myData.leftJoystickX);
    Serial.print("Joystick Y: ");
    Serial.println(myData.leftJoystickY);
  } else {
    if (TESTING_ACTIVE) {
      Serial.println("Fehler beim Senden der Daten");
    }
  }

  delay(100);   // remove for realtime actualisation
}



/*
Copyright (c) 2024 Kramperger Stefan/Meissl Alexander

Permission is hereby granted, free of charge, to any person 
obtaining a copy of this software and associated documentation 
files (the "Software"), to deal in the Software without 
restriction, including without limitation the rights to use, copy, 
modify, merge, publish, distribute, sublicense, and/or sell copies 
of the Software, and to permit persons to whom the Software is 
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be 
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN 
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/