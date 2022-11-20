#include <Arduino.h>
#include <AccelStepper.h>
#include <ServerHeader.h>

#define EVERY_MS(x) \
  static uint32_t tmr;\
  bool flag = millis() - tmr >= (x);\
  if (flag) tmr = millis();\
  if (flag)

int analogHallPin = A0;
int digitalHallPin = D7;
long homePosition = 0;
int digitalVal = 5;
bool calibrationFlag = true;
bool needToNotify = true;

struct {
    int digitalVal = digitalVal;
    long targetPosition = 0;
    long homePosition = homePosition;
    long currentPosition = homePosition;
    bool calibrationFlag = calibrationFlag;
    bool isRunning = stopFlag;
    bool stopFlag = stopFlag; 
    float speed = 0;
} previousState, currentState;



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
  Serial.println("Stopped Motor And Disabled Outputs!");
  stopFlag = true;
  wasGoing = false;


  stepper.instantStop();
  // stepper.stop();
  stepper.disableOutputs();
  needToNotify = true;

}

void handleStartMotor(){
  Serial.println("Started Motor And Enabled Outputs!");
  wasGoing = true;
  stopFlag = false;

  stepper.enableOutputs();

  // stepper.setMaxSpeed(maxSpeed); //500  1300
  // stepper.setAcceleration(acceleration); //100  1500
  needToNotify = true;
}



IRAM_ATTR void isrStop(){
  Serial.println("Rising isr!");
  if (calibrationFlag && !stopFlag){
    calibrationFlag =false;
    Serial.print("Calibration Finished!");
    homePosition = stepper.currentPosition();
    Serial.printf("\tDistance: %ld\n", homePosition);
  }
  handleStopMotor();
  needToNotify = true;
}

void verifyCalibration(){
  if(calibrationFlag){
    stepper.setSpeed(lowSpeed);
    handleStartMotor();
    // return;
  }
}

void handleMoveTo(long pos){
  verifyCalibration();
  if (pos>=0 && pos <= homePosition && stepper.currentPosition() != pos) {
    handleStartMotor();
    stepper.moveTo(pos);
  }
}



void handleButtonUp(){
  Serial.println("Up Button Pressed!");
  verifyCalibration(); 
  if (stepper.currentPosition() < homePosition) {
    handleStartMotor();
    stepper.moveTo(homePosition);
  }
}

IRAM_ATTR void isrButtonUp(){
  handleButtonUp();
}

void handleButtonSet(){
  Serial.println("Set Button Pressed!");
  handleStopMotor();
}

IRAM_ATTR void isrButtonSet(){
  handleButtonSet();
}

void handleButtonDown(){
  Serial.println("Down Button Pressed!");
  if (stepper.currentPosition() > 0) {
    handleStartMotor();
    stepper.moveTo(0);
  }
}

IRAM_ATTR void isrButtonDown(){
  handleButtonDown();
}

String processor(const String& var){
  Serial.println(var);
  if(var == "STATE"){
    if (stepper.currentPosition()){
      return "OPENNED";
    }
    else{
      return "CLOSED";
    }
  }
  // return String("???");
  return var;
}

void updateCurrentState(){
  currentState.digitalVal = digitalVal;
  currentState.targetPosition = stepper.targetPosition();
  currentState.homePosition = homePosition;
  currentState.currentPosition = stepper.currentPosition();
  currentState.calibrationFlag = calibrationFlag;
  currentState.isRunning = stepper.isRunning();
  currentState.stopFlag = stopFlag; 
  currentState.speed = stepper.speed();
}

void updatePreviousState(){
  previousState.digitalVal = currentState.digitalVal;
  previousState.targetPosition = currentState.targetPosition;
  previousState.homePosition = currentState.homePosition;
  previousState.currentPosition = currentState.currentPosition;
  previousState.calibrationFlag = currentState.calibrationFlag;
  previousState.isRunning = currentState.isRunning;
  previousState.stopFlag = currentState.stopFlag;
  previousState.speed = currentState.speed; 
}

void compareStates(){
  if (
    currentState.digitalVal      != previousState.digitalVal      ||
    currentState.targetPosition  != previousState.targetPosition  ||
    currentState.homePosition    != previousState.homePosition    ||
    currentState.currentPosition != previousState.currentPosition ||
    currentState.calibrationFlag != previousState.calibrationFlag ||
    currentState.isRunning       != previousState.isRunning       ||
    currentState.stopFlag        != previousState.stopFlag        ||
    currentState.speed           != previousState.speed 
    ) 
  {
    needToNotify = true;
  }
}

String getStateMessage(){
  // int targetPos100 = map(stepper.targetPosition(), 0, homePosition, 0, 100);

  String message = "{\"curtain\":{\"state\":" + String(stepper.isRunning()) 
                    + ",\"currentPos\":"+ stepper.currentPosition() 
                    + ",\"targetPos\":" + stepper.targetPosition()
                    + ",\"stopFlag\":" + stopFlag
                    + ",\"speed\":" + stepper.speed()
                    + ",\"isRunning\":" + stepper.isRunning()
                    + ",\"calibrationFlag\":" + calibrationFlag
                    + ",\"homepoint\":" + homePosition
                    // + ",\"targetPos100\":" + targetPos100
                    + "}}";
  return message;
}

void notifyClients() {
  String message = getStateMessage();
  ws.textAll(message);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  String message = (char*)data;
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "goUp") == 0) {
      handleButtonUp();
    }

    if (strcmp((char*)data, "goDown") == 0) {
      handleButtonDown();
    }

    if (strcmp((char*)data, "stop") == 0) {
      handleButtonSet();
    }
    needToNotify = true;

    // if (strcmp((char*)data, "toggle") == 0) {
    //   ledStrip.toggleState();
    // }

    if (message.indexOf("1s") >= 0) {
      int messageInt = message.substring(2).toInt();
      messageInt = 100 - messageInt;
      if (homePosition){
        long targetPosition = map(messageInt, 0, 100, 0, homePosition);
        handleMoveTo(targetPosition);

        Serial.printf("Target position from slider: %d, mapped: %ld.\n", messageInt, targetPosition);
      } else {
        Serial.println("Please set homeposition first!");
      }
    }

    // if (message.indexOf("2s") >= 0) {
    //   int messageInt = message.substring(2).toInt();
    //   int newMaxBrightness = map(messageInt, 0, 100, 0, 255);
    //   ledStrip.setMaxBrightness(newMaxBrightness);
         
    //   Serial.printf("Max brightness from slider: %d, mapped: %d.\n", messageInt, newMaxBrightness);
    // }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
    switch (type) {
      case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        ws.text(client->id(), getStateMessage());
        break;
      case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
      case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
      case WS_EVT_PONG:
      case WS_EVT_ERROR:
        break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
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


  WiFi.mode(WIFI_AP_STA);
  WiFi.hostname(newHostname.c_str());
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  initWebSocket();

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    // request->send_P(200, "text/html", index_html, processor);
    request->send_P(200, "text/html", index_html);
  });

  // Start server
  server.begin();
}

void loop() {
  ws.cleanupClients();
  // int analogVal = analogRead(analogHallPin);
  digitalVal = digitalRead(digitalHallPin);
  
  // EVERY_MS(100){
  //   Serial.print(stopFlag ? "stop" : "run");
  //   Serial.print("\t");
  //   Serial.print(stepper.currentPosition());
  //   Serial.print("\t");
  //   Serial.print(digitalVal);
  //   Serial.print("\t");
  //   Serial.print(stepper.speed());
  //   Serial.print("\t");
  //   Serial.print(calibrationFlag); 
  //   Serial.print("\t");
  //   Serial.print(stepper.isRunning()); 
  //   Serial.print("\t");
  //   Serial.print(stepper.targetPosition());
  //   Serial.println();
  // }

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


  EVERY_MS(300){
    updateCurrentState();
    compareStates();
    if (needToNotify){
      if (ws.count() > 0){
        notifyClients();
        needToNotify = false;
        updatePreviousState();
      }
    }
  }
}