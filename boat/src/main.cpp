//========================================================================================
// Boat FAS Project
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
#include <ESP32Servo.h>

//========================================================================================
// Configuration Constants
//========================================================================================
static constexpr bool TESTING_ACTIVE = false;
static constexpr uint8_t ESC_PIN = 16;
static constexpr uint8_t SERVO_PIN = 17;
static constexpr uint8_t ANGLE_OFFSET = 25;       // Servo angle limits (25° - 155°)
static constexpr uint8_t SERVO_OFFSET = 50;       // Servo has jitter in default position
static constexpr uint16_t MATH_MIDPOINT = 2048;   // midpoint from values 0 to 4095
static constexpr uint16_t SPEED_LIMITER = 300;    // ranges from 0 to 500 --- 0 being fastest/ 500 being slowest
static constexpr uint8_t NEUTRAL_OFFSET = 20;
static constexpr uint32_t DATA_TIMEOUT_MS = 1000; // Safety timeout
static constexpr bool CUSTOM = false;

//========================================================================================
// Data Structures
//========================================================================================
#pragma pack(push, 1)
struct ReceivedData {
    char message[32];
    float joystickX;
    float joystickY;
    bool joystickButton;
    uint16_t rotaryEncoderValue;
    bool rotaryEncoderButton;
};

struct ValueState {
    uint16_t escValue = 1500;
    uint16_t servoValue = 0;
    bool dataReceived = false;
    uint32_t lastUpdate = 0;
    unsigned int servoAngle = 90;
    const uint16_t midpoint = 1860;
    uint16_t oldAngle = 0;
};
#pragma pack(pop)

//========================================================================================
// Global Objects
//========================================================================================
ReceivedData rxData = {};
ValueState valueState;
Servo esc;
Servo angleServo;

//========================================================================================
// Function Prototypes
//========================================================================================
void onReceive(const uint8_t *mac, const uint8_t *data, int len);
void initializePeriphials();
void initializeESPNOW();
void updateESC();
void updateServo();
void safetyCheck();

//========================================================================================
// Setup & Loop
//========================================================================================
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  initializePeriphials();
  initializeESPNOW();
}

void loop() {
  safetyCheck();
    if (valueState.dataReceived) {
        updateESC();
        updateServo();
    }
    if (CUSTOM) {
        Serial.print("Y Value: ");
        Serial.println(4095 - rxData.joystickY);
        Serial.println(valueState.servoAngle);
        delay(200);
  }
}

//========================================================================================
// Hardware Initialization
//========================================================================================
void initializePeriphials(){
    angleServo.attach(SERVO_PIN);
    esc.attach(ESC_PIN, 1000, 2000);
    esc.writeMicroseconds(1500);        // neutral-position
    delay(2000);
}

void initializeESPNOW(){
    if (esp_now_init() != ESP_OK) {
      Serial.println("ESP-NOW init failed");
      while(1);
    }
    
    esp_now_register_recv_cb(onReceive);
    memset(&rxData, 0, sizeof(rxData)); // Ensure clean start
}

void onReceive(const uint8_t *mac, const uint8_t *data, int len) {
  if (len == sizeof(rxData)) {
    memcpy(&rxData, data, len);
    valueState.dataReceived = true;
    valueState.lastUpdate = millis();
    
    if (TESTING_ACTIVE) {
      Serial.printf("Received: X=%.1f Y=%.1f Enc=%u\n", 
                   rxData.joystickX,
                   rxData.joystickY,
                   rxData.rotaryEncoderValue);
    }
  }
}

//========================================================================================
// ESC + Servo Update and Safety Features
//========================================================================================
void updateESC(){
    if (rxData.rotaryEncoderValue > 1500)      {valueState.escValue = constrain(rxData.rotaryEncoderValue, 1500, 2000 - SPEED_LIMITER - NEUTRAL_OFFSET);}
    else if (rxData.rotaryEncoderValue < 1500) {valueState.escValue = constrain(rxData.rotaryEncoderValue, 1000 + SPEED_LIMITER, 1500);}
    else {
        valueState.escValue = 1500;
    }
    esc.writeMicroseconds(valueState.escValue + NEUTRAL_OFFSET);
}

void updateServo(){
    if (rxData.joystickY < MATH_MIDPOINT - SERVO_OFFSET || rxData.joystickY > MATH_MIDPOINT + SERVO_OFFSET) {
        if (rxData.joystickY < valueState.midpoint) {
            valueState.servoAngle = map(4095 - rxData.joystickY, 0, 4095 - valueState.midpoint, ANGLE_OFFSET, 90);
        } else if (rxData.joystickY > valueState.midpoint){
            valueState.servoAngle = map(4095 - rxData.joystickY, 4095 - valueState.midpoint, 4095, 90, 180 - ANGLE_OFFSET);
        } else {  // servoAngle == midpoint
            valueState.servoAngle = 90;
        }
    }
    angleServo.write(valueState.servoAngle);
}

// In case of date timeout, return to neutral positions and stop the motor
void safetyCheck() {
    if (millis() - valueState.lastUpdate > DATA_TIMEOUT_MS) {
        esc.writeMicroseconds(1500 + NEUTRAL_OFFSET);
        angleServo.write(90);
        valueState.dataReceived = false;
    }
}