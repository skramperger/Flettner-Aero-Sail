//========================================================================================
// Controller FAS Project
//========================================================================================

#include <esp_now.h>
#include <WiFi.h>
#include <Arduino.h>
#include <math.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

uint8_t ControllerAddress[] = {0xFC, 0xB4, 0x67, 0x77, 0x9B, 0x5C};
uint8_t BoatAddress[] =       {0xE4, 0x65, 0xB8, 0x49, 0xC3, 0x68};

#define RESOLUTION 12
#define TESTING_ACTIVE false

#define VRX_PIN  39 // VN 
#define VRY_PIN  36 // VP

// Rotary Encoder Pins
#define ENCODER_PIN_A 32    // CLK Pin
#define ENCODER_PIN_B 33    // DT Pin
#define ENCODER_BTN   25    // optional push button pin

// SDA and SCL Pins
#define SDA_PIN 21
#define SCL_PIN 22

// OLED Display Config
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1 // not used with ESP32

// Create the display object (I2C)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Structure for Data Transmission
typedef struct struct_data {
  char  message[32];
  float JoystickX;
  float JoystickY;
  bool  JoystickButton;
  uint16_t RotaryEncoderValue;
  bool  RotaryEncoderButton;
} struct_data;

struct_data myData;

typedef struct struct_values {
  uint16_t X = 0;
  uint16_t Y = 0;
  float    ModifiedX = 0.0;
  float    ModifiedY = 0.0;
  volatile long encoderValue = 0;
  volatile long lastEncoderValue = 0;
  int buttonState = HIGH;
  int lastButtonState = HIGH;
  unsigned long lastDebounceTime = 0;
  unsigned long debounceDelay = 50; // milliseconds
  bool reverseState = false;
  bool dataReady = true;
  long speed = 0;
} struct_values;

struct_values Values;

void IRAM_ATTR updateEncoderA();
void IRAM_ATTR updateEncoderB();

//========================================================================================
// Functions
//========================================================================================
void onSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (TESTING_ACTIVE) {
    Serial.print("Nachricht gesendet: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Erfolg" : "Fehlgeschlagen");
  }
}

/**
 * Function that remaps the Joystick Values before they are sent via ESP-NOW
 */
void updateJoystickValue() {
  Values.X = analogRead(VRX_PIN);   // try digitalread
  Values.Y = analogRead(VRY_PIN);
  Values.ModifiedX = map(Values.X, 0, 4095, -255, 304);    // -208 255
  Values.ModifiedY = map(Values.Y, 0, 4095, 255, -306.5);  // 255 -310.5
  Values.ModifiedX = constrain(Values.ModifiedX, -255, 255);
  Values.ModifiedY = constrain(Values.ModifiedY, -255, 255);
  myData.JoystickX = Values.ModifiedX;
  myData.JoystickY = Values.ModifiedY;
}
//========================================================================================


//========================================================================================
// Setup
//========================================================================================
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);


  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), updateEncoderA, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), updateEncoderB, CHANGE);
  Wire.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Most 128x32 OLEDs are at I2C address 0x3C
    Serial.println("SSD1306 allocation failed");
    while (true); // Don’t proceed, loop forever
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Flettner Aero Sail");
  display.display();
  Serial.println("Setup complete.");


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
//========================================================================================


//========================================================================================
// Loop
//========================================================================================
void loop() {
  if (Values.dataReady) {
    updateJoystickValue();

    esp_err_t result = esp_now_send(BoatAddress, (uint8_t *) &myData, sizeof(myData));

    if (result == ESP_OK && TESTING_ACTIVE) {
      Serial.println("Daten erfolgreich gesendet");
      Serial.print("Joystick X: ");
      Serial.println(myData.JoystickX);
      Serial.print("Joystick Y: ");
      Serial.println(myData.JoystickY);
      delay(1000);
    } else {
      if (TESTING_ACTIVE) {
        Serial.println("Fehler beim Senden der Daten");
        delay(1000);
      }
    }

  int reading = digitalRead(ENCODER_BTN);
  
  if (reading != Values.lastButtonState) {
    Values.lastDebounceTime = millis();
  }

  if ((millis() - Values.lastDebounceTime) > Values.debounceDelay) {
    // Only update if the state has changed
    if (reading != Values.buttonState) {
      Values.buttonState = reading;

      // Trigger on the FALLING edge (button pressed)
      if (Values.buttonState == LOW) {
        Values.reverseState != Values.reverseState;
        if (TESTING_ACTIVE) {
          Serial.println("Button pressed!");
          }
        if (Values.reverseState == true) {
          myData.RotaryEncoderValue = map(Values.speed, 0, 100, 1500, 2000);
        } else {
          myData.RotaryEncoderValue = map(Values.speed, 0, 100, 1500, 1000);
        }
      }
    }
  }

  Values.lastButtonState = reading;


  if (Values.encoderValue != Values.lastEncoderValue) {
    Values.lastEncoderValue = Values.encoderValue;

    display.clearDisplay();

    Values.speed = Values.encoderValue; 
    if (Values.speed < 0)   Values.speed = 0;
    if (Values.speed > 100) Values.speed = 100;   // maybe use constrain() instead? .. dunno

    // Map speed 0–100 to bar width 0–128
    int barWidth = map(Values.speed, 0, 100, 0, SCREEN_WIDTH);

    display.setCursor(0, 0);
    display.print("Speed: ");
    display.print(Values.speed);
    display.print("%");

    int barHeight = 10;
    int barY      = 16;
    display.fillRect(0, barY, barWidth, barHeight, WHITE);

    // Update the display
    display.display();
    }
  }
}
//========================================================================================


//========================================================================================
// ISRs
//========================================================================================
void IRAM_ATTR updateEncoderA() {
  bool A = digitalRead(ENCODER_PIN_A);
  bool B = digitalRead(ENCODER_PIN_B);

  // Determine the direction
  if (A == B) {
    Values.encoderValue--;
  } else {
    Values.encoderValue++;
  }
}

void IRAM_ATTR updateEncoderB() {
  bool A = digitalRead(ENCODER_PIN_A);
  bool B = digitalRead(ENCODER_PIN_B);

  // Determine the direction
  if (A != B) {
    Values.encoderValue--;
  } else {
    Values.encoderValue++;
  }
}
//========================================================================================