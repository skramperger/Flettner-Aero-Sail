//========================================================================================
// Boat FAS Project
//========================================================================================

#include <esp_now.h>
#include <WiFi.h>
#include <ESP32Servo.h>

static constexpr bool TESTING_ACTIVE = false;
static constexpr bool REVERSE_ACTIVE = true;
static constexpr uint8_t ESC_PIN = 16;
static constexpr uint8_t SERVO_PIN = 17;
static constexpr uint8_t ANGLE_OFFSET = 25;   // offset in degrees
static constexpr uint8_t SPEED_LIMITER = 0; // ranges from 0 to 500 --- 0 being fastest/ 500 being slowest
static constexpr uint8_t NEUTRAL_OFFSET = 20;

Servo esc;
Servo angleServo;

#pragma pack(push, 1)
typedef struct struct_data {
  char message[32];
  float joystickX;
  float joystickY;
  bool joystickButton;
  uint16_t rotaryEncoderValue;
  bool rotaryEncoderButton;
} struct_data;

struct ValueState {
  uint16_t escValue = 1500;
  uint16_t servoValue = 0;
  bool dataReceived = false;
  unsigned int angle = 0;
  uint16_t midpoint = 2048;
  uint16_t oldAngle = 0;
};

#pragma pack(pop)

struct_data incomingData = {};
ValueState valueState;

//========================================================================================
// Callbacks & Functions
//========================================================================================
void onReceive(const uint8_t *mac, const uint8_t *data, int len) {
  if (len == sizeof(incomingData)) {
    memcpy(&incomingData, data, len);
    valueState.dataReceived = true;
    
    if (TESTING_ACTIVE) {
      Serial.printf("Received: X=%.1f Y=%.1f Enc=%u\n", 
                   incomingData.joystickX,
                   incomingData.joystickY,
                   incomingData.rotaryEncoderValue);
    }
  }
}

float allowReverse(float value, bool allowance) {
  return allowance ? value : fmaxf(value, 0.0);
}

//========================================================================================
// Setup & Loop
//========================================================================================
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  angleServo.attach(SERVO_PIN);

  esc.attach(ESC_PIN, 1000, 2000);
  esc.writeMicroseconds(1500);
  delay(2000);



  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while(1);
  }
  
  esp_now_register_recv_cb(onReceive);
  memset(&incomingData, 0, sizeof(incomingData)); // Ensure clean start
}

void loop() {
  if (!valueState.dataReceived) return;

  // ESC
  if (incomingData.rotaryEncoderValue > 1500) {valueState.escValue =      constrain(incomingData.rotaryEncoderValue, 1500, 2000 - NEUTRAL_OFFSET);}
  else if (incomingData.rotaryEncoderValue < 1500) {valueState.escValue = constrain(incomingData.rotaryEncoderValue, 1000, 1500);}
  else {
    valueState.escValue = 1500;
  }

  esc.writeMicroseconds(valueState.escValue + NEUTRAL_OFFSET);

  // Servo 
  if (incomingData.joystickX < valueState.midpoint) {
    valueState.angle = map(incomingData.joystickX, 0, valueState.midpoint, ANGLE_OFFSET, 90);
  } else {
    valueState.angle = map(incomingData.joystickX, valueState.midpoint, 4095, 90, 180 - ANGLE_OFFSET);
  }
  valueState.oldAngle = valueState.angle;
  
  if (valueState.oldAngle - 1 == valueState.angle || valueState.oldAngle + 1 == valueState.angle){
    
  }
  angleServo.write(valueState.angle);
}