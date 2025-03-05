//========================================================================================
// Controller FAS Project
//========================================================================================

// To-Do: 
//----------------------------------------------------------------------------------------
// Urgent:
//        decrementing speed control 1500 stops it immediately → millis() function for timing  
//        let the neutral_offset be 0, when no data has been received
//----------------------------------------------------------------------------------------
// Ideas: 
//        implement a speedometer for the rotating cylinder and a corresponding display output
//        implement a maximum speed limiter (example: limit to 1750 instead of 2000)
//        implement sleep modes for power efficiency
//----------------------------------------------------------------------------------------

#include <esp_now.h>
#include <WiFi.h>
#include <Arduino.h>
#include <math.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//========================================================================================
// Configuration Constants
//========================================================================================
static const uint8_t CONTROLLER_MAC[] = {0xFC, 0xB4, 0x67, 0x77, 0x9B, 0x5C};
static const uint8_t BOAT_MAC[] =       {0xE4, 0x65, 0xB8, 0x49, 0xC3, 0x68};

static constexpr uint8_t JOYSTICK_X_PIN = 39;   // VRX VN
static constexpr uint8_t JOYSTICK_Y_PIN = 36;   // VRY VP
static constexpr uint8_t ENCODER_CLK = 32;      // A
static constexpr uint8_t ENCODER_DT = 33;       // B
static constexpr uint8_t ENCODER_BTN = 25;
static constexpr uint8_t SDA_PIN = 21;
static constexpr uint8_t SCL_PIN = 22;

static constexpr bool TESTING_ACTIVE = false;
static constexpr bool CUSTOM = false;
static constexpr unsigned long DEBOUNCE_DELAY_MS = 50;

// Display configuration
static constexpr uint8_t SCREEN_WIDTH = 128;
static constexpr uint8_t SCREEN_HEIGHT = 32;
static constexpr uint8_t OLED_ADDRESS = 0x3C;

//========================================================================================
// Data Structures
//========================================================================================
#pragma pack(push, 1)
// Structure for Data Transmission
struct TransmitData {
    char  message[32];
    float joystickX;
    float joystickY;
    bool  joystickButton;
    uint16_t rotaryEncoderValue;
    bool  rotaryEncoderButton;
};

struct ValueState {
    uint16_t rawX = 0;
    uint16_t rawY = 0;
    float    processedX = 0.0;
    float    processedY = 0.0;
    volatile long encoderValue = 0;
    volatile long lastEncoderValue = 0;
    int buttonState = HIGH;
    int lastButtonState = HIGH;
    unsigned long lastDebounceTime = 0;
    bool reverseState = false;
    bool dataReady = true;
    long speed = 0;
};
#pragma pack(pop)

//========================================================================================
// Global Objects
//========================================================================================
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
TransmitData txData;
ValueState valueState;

//========================================================================================
// Function Prototypes
//========================================================================================
void initializeDisplay();
void initializeEncoder();
void initializeESPNOW();
void updateJoystickValues();
void handleEncoderButton();
void updateDisplay();
void sendControlData();
void resetValues();
void onSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void IRAM_ATTR updateEncoderA();
void IRAM_ATTR updateEncoderB();

//========================================================================================
// Setup and Loop
//========================================================================================
void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);

    initializeDisplay();
    initializeEncoder();
    initializeESPNOW();

    strncpy(txData.message, "Operational", sizeof(txData.message)-1);
    txData.message[sizeof(txData.message) - 1] = '\0';
    Serial.println("System initialized");
}

void loop() {
    if (valueState.dataReady) {
        updateJoystickValues();
        handleEncoderButton();
        updateDisplay();
        sendControlData();
    }

    if (CUSTOM) {
        Serial.print("X Value: ");
        Serial.println(txData.joystickX);
        Serial.print("Y Value: ");
        Serial.println(txData.joystickY);
        delay(200);
  }
}

//========================================================================================
// Hardware Initialization
//========================================================================================
void initializeDisplay() {
    Wire.begin();
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        Serial.println("Display initialization failed");
        while(true);
    }
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Flettner Aero Sail");
    display.display();
}

void initializeEncoder() {
    pinMode(ENCODER_CLK, INPUT_PULLUP);
    pinMode(ENCODER_DT, INPUT_PULLUP);
    pinMode(ENCODER_BTN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), updateEncoderA, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_DT), updateEncoderB, CHANGE);
    txData.rotaryEncoderValue = 1500;
}

void initializeESPNOW() {
    if (esp_now_init() != ESP_OK && TESTING_ACTIVE) {
      Serial.println("ESP-NOW konnte nicht initialisiert werden");
      return;
    }
    esp_now_register_send_cb(onSent);
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, BOAT_MAC, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (!esp_now_is_peer_exist(BOAT_MAC)) {
      if (esp_now_add_peer(&peerInfo) != ESP_OK && TESTING_ACTIVE) {
        Serial.println("Peer registration failed");
      }
    }
}

//========================================================================================
// Input Handling
//========================================================================================
void updateJoystickValues() {
    valueState.rawX = analogRead(JOYSTICK_X_PIN);
    valueState.rawY = analogRead(JOYSTICK_Y_PIN);

    // 
    valueState.processedX = constrain(
        map(valueState.rawX, 0, 4095, -255, 304), -255, 255);   // -208 255
    valueState.processedY = constrain(
        map(valueState.rawY, 0, 4095, 255, -306.5), -255, 255); // 255 -310.5

    txData.joystickX = valueState.rawX;
    txData.joystickY = valueState.rawY;
}

void handleEncoderButton() {
    int currentState = digitalRead(ENCODER_BTN);
    
    if (currentState != valueState.lastButtonState) {
        valueState.lastDebounceTime = millis();
    }
    if ((millis() - valueState.lastDebounceTime) > DEBOUNCE_DELAY_MS) {
        if (currentState != valueState.buttonState) {
            valueState.buttonState = currentState;

            // Trigger on the FALLING edge (button pressed)
            if (valueState.buttonState == LOW) {
                valueState.reverseState = !valueState.reverseState;
                resetValues();
                if (valueState.reverseState == true) {
                    txData.rotaryEncoderValue = map(valueState.speed, 0, 100, 1500, 2000);
                } else {
                    txData.rotaryEncoderValue = map(valueState.speed, 0, 100, 1500, 1000);
                }
            }
        }
    }
    valueState.lastButtonState = currentState;
}

void resetValues(){
    valueState.speed = 0;
    valueState.encoderValue = 0;
    valueState.lastEncoderValue = 0;
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Speed: ");
    display.print(valueState.speed);
    display.print("%");
    display.display();
}

//========================================================================================
// Display & Communication
//========================================================================================
void updateDisplay() {
    if (valueState.encoderValue != valueState.lastEncoderValue) {
        valueState.lastEncoderValue = valueState.encoderValue;

        display.clearDisplay();

        valueState.speed = constrain(valueState.encoderValue, 0, 100);
        
        // Update PWM value based on current speed and direction
        txData.rotaryEncoderValue = valueState.reverseState 
            ? map(valueState.speed, 0, 100, 1500, 2000)
            : map(valueState.speed, 0, 100, 1500, 1000);

        // Map speed 0–100 to bar width 0–128
        int barWidth = map(valueState.speed, 0, 100, 0, SCREEN_WIDTH);

        display.setCursor(0, 0);
        display.printf("Speed: %d%%", valueState.speed);

        int barHeight = 10;
        int barY      = 16;
        display.fillRect(0, barY, barWidth, barHeight, WHITE);

        // Update the display
        display.display();
    }
}

void sendControlData() {
    esp_err_t result = esp_now_send(BOAT_MAC, (uint8_t *) &txData, sizeof(txData));

    if (result != ESP_OK && TESTING_ACTIVE) {
      Serial.println("Transmission failed");
    } 
}

void onSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (TESTING_ACTIVE) {
    Serial.print("Message sent: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Failure");
  }
}

//========================================================================================
// Encoder ISRs
//========================================================================================
void IRAM_ATTR updateEncoderA() {
  const bool A = digitalRead(ENCODER_CLK);
  const bool B = digitalRead(ENCODER_DT);

  valueState.encoderValue += (A == B) ? -1 : 1;
  valueState.encoderValue = constrain(valueState.encoderValue, 0, 100);
}

void IRAM_ATTR updateEncoderB() {
  bool A = digitalRead(ENCODER_CLK);
  bool B = digitalRead(ENCODER_DT);

  valueState.encoderValue += (A != B) ? -1 : 1;
  valueState.encoderValue = constrain(valueState.encoderValue, 0, 100);
}