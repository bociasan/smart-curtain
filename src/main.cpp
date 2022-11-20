#include <Arduino.h>
#include <AccelStepper.h>

#define EVERY_MS(x) \
  static uint32_t tmr;\
  bool flag = millis() - tmr >= (x);\
  if (flag) tmr = millis();\
  if (flag)

int analogHallPin = A0;
int digitalHallPin = D7;
int homePosition = 0;

bool calibrationFlag = true;

#define ButtonUp   D2
#define ButtonSet  D1
#define ButtonDown D6

#define IN1 D0 //5  D1
#define IN2 D3 //4  D2
#define IN3 D5//14 D5
#define IN4 D8//12 D6
AccelStepper stepper(AccelStepper::HALF4WIRE, IN1, IN3, IN2, IN4);

int maxSpeed = 1400;
int lowSpeed = 1000;
int acceleration = 1500;

bool stopFlag = true;
bool wasGoing = false;

void handleStopMotor(){
  Serial.println("Stoped Motor And Disabled Outputs!");
  stopFlag = true;
  wasGoing = false;


  stepper.instantStop();
  // stepper.stop();
  stepper.disableOutputs();

}

void handleStartMotor(){
  wasGoing = true;
  stopFlag = false;

  stepper.enableOutputs();

  // stepper.setMaxSpeed(maxSpeed); //500  1300
  // stepper.setAcceleration(acceleration); //100  1500

}



IRAM_ATTR void isrStop(){
  Serial.println("Rising isr!");
  if (calibrationFlag){
    calibrationFlag =false;
    Serial.print("Calibration Finished!");
    homePosition = stepper.currentPosition();
    Serial.printf("\tDistance: %d\n", homePosition);
  }
  handleStopMotor();
}

IRAM_ATTR void isrButtonUp(){
  Serial.println("Up Button Pressed!");
  if(calibrationFlag){
    stepper.setSpeed(lowSpeed);
    handleStartMotor();
    // return;
  } 

  if (stepper.currentPosition() < homePosition) {
    handleStartMotor();
    stepper.moveTo(homePosition);
  }

}

IRAM_ATTR void isrButtonSet(){
  Serial.println("Set Button Pressed!");
  handleStopMotor();
}

IRAM_ATTR void isrButtonDown(){
  Serial.println("Down Button Pressed!");
if (stepper.currentPosition() > 0) {
      handleStartMotor();
      stepper.moveTo(0);
    }
}

void setup() {
  pinMode(digitalHallPin, INPUT);
  pinMode(ButtonUp,   INPUT_PULLUP);
  pinMode(ButtonSet,  INPUT_PULLUP);
  pinMode(ButtonDown, INPUT_PULLUP);
  // stepper.setMaxSpeed(maxSpeed); //500
  // stepper.setAcceleration(acceleration); //100
  // stepper.setSpeed(speed); 


  stepper.setMaxSpeed(maxSpeed); //500  1300
  stepper.setAcceleration(acceleration); //100  1500

  attachInterrupt(digitalHallPin, isrStop, RISING);  

  attachInterrupt(ButtonUp, isrButtonUp, RISING);  
  attachInterrupt(ButtonSet, isrButtonSet, RISING);  
  attachInterrupt(ButtonDown, isrButtonDown, RISING);  

  Serial.begin(9600);

  // stepper.setSpeed(lowSpeed); //1500
}

void loop() {
  // int analogVal = analogRead(analogHallPin);
  int digitalVal = digitalRead(digitalHallPin);
  
  EVERY_MS(100){
    Serial.print(stopFlag ? "stop" : "run");
    Serial.print("\t");
    Serial.print(stepper.currentPosition());
    Serial.print("\t");
    Serial.print(digitalVal);
    Serial.print("\t");
    Serial.print(stepper.speed());
    Serial.print("\t");
    Serial.print(calibrationFlag); 
    Serial.print("\t");
    Serial.print(stepper.isRunning()); 
    Serial.print("\t");
    Serial.print(stepper.targetPosition());

    Serial.println();
  }

  // EVERY_MS(100){
  //   Serial.printf("%d\t%d\t%d\n", digitalRead(ButtonDown), digitalRead(ButtonUp), digitalRead(ButtonSet));
  // }


  if (!stopFlag){
      if (calibrationFlag){
        stepper.runSpeed();
      } else {
        stepper.run();
      }
  } 
  // else {
  //   stepper.setSpeed(0);
  //   stepper.stop();
  //   stepper.disableOutputs();
  // }

  if (homePosition && wasGoing && stepper.distanceToGo()  == 0) {
    handleStopMotor();
  }
}