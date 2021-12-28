#include <Arduino.h>

#include <HardwareSerial.h>

#include <TMCStepper.h>
#include "SpeedyStepper.h"


#define DIAG_PIN         18           // STALL motor 2
#define EN_PIN           5            // Enable
#define DIR_PIN          23           // Direction
#define STEP_PIN         22           // Step
#define SERIAL_PORT      Serial2      // TMC2208/TMC2224 HardwareSerial port
#define DRIVER_ADDRESS   0b00         // TMC2209 Driver address according to MS1 and MS2
#define R_SENSE          0.11f        // R_SENSE for current calc.  
/*
  Higher STALL_VALUE value -> higher torque to indicate the stall and vice versa
  Lower  STALL_VALUE value -> lower torque to indicate stall detection
  the diag_pin will be 1 when SG_THRS*2 > SG_RESULT 
*/
#define STALL_VALUE      30       // [0..255]

bool shaft = false; // for control through the driver

hw_timer_t * timer1 = NULL;
TMC2209Stepper driver(&SERIAL_PORT, R_SENSE , DRIVER_ADDRESS);
using namespace TMC2208_n;

SpeedyStepper stepper;

const int ledPin = 2;

void homing() {
  Serial.println("Homing initialized...");
  // set up the speed and accel
  stepper.setSpeedInStepsPerSecond(800);
  stepper.setAccelerationInStepsPerSecondPerSecond(400);
  stepper.setCurrentPositionInSteps(0);
  stepper.setupMoveInSteps(-5000);
  // command servo to move until diag_bool is 1
  while (!stepper.motionComplete()) {
    stepper.processMovement();
    // bool diag_value = digitalRead(DIAG_PIN);
    // Serial.println(driver.SG_RESULT(), DEC);
    if (digitalRead(DIAG_PIN) == 1) {
      stepper.setupStop();
      // Serial.println("Stall Detected !");
      break;
    }
  }

  // stepper.setupMoveInSteps(0);
  stepper.setCurrentPositionInSteps(0);
  stepper.setupMoveInSteps(120);
  while (!stepper.motionComplete()) {
    stepper.processMovement();
  }
  // set current step to 0
  stepper.setCurrentPositionInSteps(0);
  stepper.setupMoveInSteps(0);
}

void setup() {
  Serial.begin(115200);         // Init serial port and set baudrate
  while(!Serial);               // Wait for serial port to connect
  Serial.println("\nStart...");
  SERIAL_PORT.begin(115200, SERIAL_8N1, 16, 17); // HW serial btw esp32-tmc2099; baud, config, rx, tx

  pinMode(DIAG_PIN ,INPUT);
  pinMode(EN_PIN ,OUTPUT);
  pinMode(STEP_PIN ,OUTPUT);
  pinMode(DIR_PIN ,OUTPUT);

  pinMode(ledPin, OUTPUT);

  digitalWrite(EN_PIN ,HIGH); // true: no output, false: output
  delay(500);
  digitalWrite(EN_PIN ,LOW);
  //digitalWrite(DIR_PIN , shaft);

  driver.begin();
  driver.toff(4);
  driver.blank_time(24);
  driver.rms_current(800); // mA max 1.5
  driver.I_scale_analog(true); // true: Use voltage supplied to VREF as current reference
  driver.microsteps(2); // up to 256 or 1/256
  driver.TCOOLTHRS(0xFFFFF); // 20bit max
  driver.semin(5);
  driver.semax(2);
  driver.sedn(0b01);
  
  //driver.shaft(false); // true=CW/open, false=CCW/close
  driver.pdn_disable(true); // true: PDN_UART input function disabled. Set this bit, when using the UART interface
  driver.pwm_autoscale(true);   // Needed for stealthChop

  delay(100);

  stepper.connectToPins(STEP_PIN, DIR_PIN);

  // check connection of esp32-tmc2209
  Serial.println("\nTesting tmc2209-esp32 connection...");
  uint8_t connection_result = driver.test_connection();
  if (connection_result) {
      Serial.println("connection failed");
      Serial.print("Likely cause: ");

      switch(connection_result) {
          case 1: Serial.println("loose connection"); break;
          case 2: Serial.println("no power"); break;
      }

      Serial.println("Fix the problem and reset board.");
      
      delay(10); // We need this delay or messages above don't get fully printed out
      abort();
  }
  Serial.println("tmc2209-esp32 connection is OK"); // if connection is okay

  driver.SGTHRS(20); // for homing
  homing();
  Serial.println("Homing Finished");

  driver.SGTHRS(STALL_VALUE); // normal operation
  driver.microsteps(2);
  Serial.println("Stepper Ready");
}



void loop() {
  
  static uint32_t last_time=0;
  uint32_t ms = millis();
  bool diag_bool = digitalRead(DIAG_PIN);

  stepper.setSpeedInStepsPerSecond(300);
  stepper.setAccelerationInStepsPerSecondPerSecond(300);
  stepper.setupMoveInSteps(1200);

  while (!stepper.motionComplete()) {
    stepper.processMovement();
    // Serial.println("moving...");
  }

  // if ((ms-last_time) > 100) {
  //   last_time = ms;

    
  // }

}
