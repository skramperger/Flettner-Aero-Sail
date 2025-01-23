//========================================================================================
// Boat FAS Project
//========================================================================================

// To-Do: implement esc driver control for fin motor

// Receiver: MAC Address: E4:65:B8:49:C3:68
// Sender: MAC Address: FC:B4:67:77:9B:5C 
// Sender (Meissl): MAC Address: A8:42:E3:B8:24:EC

#include <esp_now.h>
#include <WiFi.h>
#include <ESP32Servo.h>

#define TESTING_ACTIVE true
#define REVERSE_ACTIVE true       // false to prevent values going below 0 for pwm     
#define REVOLUTION_PERCENTAGE 50   // values represent 0-100%

#define ESC_PIN 16

Servo esc;

// Structure for Data Transmission
// Left and right represent the left or right joystick on the controller
typedef struct struct_data {
  char  message[32];
  float JoystickX;    // goes from values -255 to 255
  float JoystickY;    // goes from values -255 to 255
  bool  JoystickButton;
  uint16_t RotaryEncoderValue;
  bool  RotaryEncoderButton;
} struct_data;

struct_data incomingData;

typedef struct output_pwm_data {

} output_pwm_data;

output_pwm_data signalData; 

//========================================================================================
// Functions
//========================================================================================
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
      Serial.println(incomingData.JoystickX);
      Serial.print("Linker Joystick Y-Wert: ");
      Serial.println(incomingData.JoystickY);
      Serial.print("Button-Status: ");
      if (incomingData.JoystickButton) {
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
  return joystickValue;
}

//========================================================================================


//========================================================================================
// Setup
//========================================================================================
void setup() {
  // Register the callback function to handle received data
  esp_now_register_recv_cb(onReceive);

  esc.attach(ESC_PIN);
  esc.writeMicroseconds(1500);
  delay(1000);  // wait until ESC is initialized

  if (TESTING_ACTIVE){
    Serial.begin(115200);
  }

  esc.attach(ESC_PIN);
  esc.writeMicroseconds(1500);    // default value for stopping
  delay(1000);

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK && TESTING_ACTIVE) {

    Serial.println("ESP-NOW konnte nicht initialisiert werden");
    return;
  }
}
//========================================================================================


//========================================================================================
// Loop
//========================================================================================
void loop() {
    incomingData.JoystickX = allowReverse(incomingData.JoystickX, REVERSE_ACTIVE);
    incomingData.JoystickY = allowReverse(incomingData.JoystickY, REVERSE_ACTIVE);

    if (incomingData.RotaryEncoderValue > 1450 && incomingData.RotaryEncoderValue < 1550) {
      esc.writeMicroseconds(1500);
    } else {
      esc.writeMicroseconds(incomingData.RotaryEncoderValue);
    }
    delay(50);
}
//========================================================================================
