#include <Arduino.h>
#include  <WiFi.h>
#include  <iostream>
#include  <sstream>

#include  <ESPAsyncWebServer.h>
#include  <AsyncTCP.h>

#define UP 1
#define DOWN 2
#define LEFT 3
#define RIGHT 4
#define STOP  0

#define RIGHT_MOTOR 0
#define LEFT_MOTER  1
#define FORWARD   1
#define BACKWARD  -1

const int PWMFreq=1000; //1KHZ
const int PWMResoution  = 8 ; // 8 bits
const int PWMSpeedChannel  = 4;

const char*  ssid  = "freecom_2.4G";
const char* password  = "26083531";

AsyncWebServer  server(80);
AsyncWebSocket  wsCarInput("/CarInput");  // url 

struct MOTOR_PINS
{
  /* data */
  int pinEn;
  int pinIN1;
  int pinIN2;
};

std::vector<MOTOR_PINS>  motor_pins =
{
  {22,16,17},   // 第一組 Right motor (EnA,IN1,IN2)
  {23,18,19},   // 第二組 Left motor   (EnB,IN3,IN4)
};

void  setUpPinModes()
{
    ledcSetup(PWMSpeedChannel,PWMFreq,PWMResoution);  // PWM set up
    for (int i=0; i < motor_pins.size(); i++)
    {
      pinMode(motor_pins[i].pinEn,OUTPUT); 
      pinMode(motor_pins[i].pinIN1,OUTPUT); 
      pinMode(motor_pins[i].pinIN2,OUTPUT); 
      ledcAttachPin(motor_pins[i].pinEn,PWMSpeedChannel);  //PWM 腳位連結
    }

}
void  handleRoot(AsyncWebServerRequest *request)
{

  request->send(200,"text/html,htmlHomePage);

}

void  handleNotFound(AsyncWebServerRequest *request)
{

  request->send(404,"text/plain","File Not Found");

}
void setup() {
  // put your setup code here, to run once:
  setUpPinModes();  //self function
  Serial.begin(11520);
  WiFi.softAP(ssid,password);
  IPAddress IP= WiFi.softAP();
  Serial1.print("AP IP address");
  Serial1.println(IP);

  server.on("/",HTTP_GET,handleRoot);
  server.onNotFound(handleNotFound);

  wsCarInput.onEvent(onCarInputWebSocketEvent);
  server.addHandler(&wsCarInput); // need to know memory address
  server.begin();
  Serial.println("HTTP Server started")




}

void loop() {
  // put your main code here, to run repeatedly:
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}