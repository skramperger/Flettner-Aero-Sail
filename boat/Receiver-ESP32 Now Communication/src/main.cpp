// Receiver: MAC Address: E4:65:B8:49:C3:68
// Sender: MAC Address: FC:B4:67:77:9B:5C 
// Sender (Meissl): MAC Address: A8:42:E3:B8:24:EC

#include <esp_now.h>
#include <WiFi.h>

#define TESTING_ACTIVE true
#define REVERSE_ACTIVE false       // false to prevent values going below 0 for pwm     
#define REVOLUTION_PERCENTAGE 50   // values represent 0-100%

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

struct_data incomingData;

typedef struct output_pwm_data {

} output_pwm_data;

output_pwm_data signalData; 

// Callback function to handle received data
void onReceive(const uint8_t * mac_addr, const uint8_t *data, int len) {
  if (len == sizeof(incomingData)) {
    memcpy(&incomingData, data, sizeof(incomingData));

    String receivedMessage = String(incomingData.message);

    if (TESTING_ACTIVE){
      Serial.println("Daten empfangen:");
      Serial.print("Nachricht: ");
      Serial.println(receivedMessage);
      Serial.print("Linker Joystick X-Wert: ");
      Serial.println(incomingData.leftJoystickX);
      Serial.print("Linker Joystick Y-Wert: ");
      Serial.println(incomingData.leftJoystickY);
      Serial.print("Button-Status: ");
      if (incomingData.leftSwitch) {
        Serial.println("Knopf A gedrückt.");
      } else {
        Serial.println("Knopf A nicht gedrückt.");
      }
      }
  } else {
    if (TESTING_ACTIVE){
      Serial.println("Falsche Datenlänge empfangen!");
    }
  }
}


/**
 * Function that en-/disables reverse for the rotor
 * @param value the initial value to modify
 * @param allowance determines whether you want it true or false
 * returns a float value either negative or only positive depending on wether you allowance is true or false
 */
float allowReverse(float value, bool allowance){
  if (!allowance){
    if (value <= 0.0){value = 0.0;}
  } else {value = value;}
  return value;
}

int controlPWMSignal(float joystickValue){
  
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

  // Register the callback function to handle received data
  esp_now_register_recv_cb(onReceive);
}

void loop() {
    incomingData.leftJoystickX = allowReverse(incomingData.leftJoystickX, REVERSE_ACTIVE);
    incomingData.leftJoystickY = allowReverse(incomingData.leftJoystickY, REVERSE_ACTIVE);
}


/*
Copyright (c) <year> <copyright holders>

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