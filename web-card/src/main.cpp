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

const char* htmlHomePage  PROGMEM=R"HTMLHOMEPAGE(
<!DOCTYPE HTML>
<html>
  <head>
    <style>
    .arrows {
      font-size:40px;
      color:red;
    }
    td.button {
      background-color:black;
      border-radius:25%;
      box-shadow: 5px 5px #888888;
    }
    td.button:active {
      transform: translate(5px,5px);
      box-shadow: none; 
    }

    .noselect {
      -webkit-touch-callout: none; /* iOS Safari */
        -webkit-user-select: none; /* Safari */
         -khtml-user-select: none; /* Konqueror HTML */
           -moz-user-select: none; /* Firefox */
            -ms-user-select: none; /* Internet Explorer/Edge */
                user-select: none; /* Non-prefixed version, currently
                                      supported by Chrome and Opera */
    }

    .slidecontainer {
      width: 100%;
    }

    .slider {
      -webkit-appearance: none;
      width: 100%;
      height: 20px;
      border-radius: 5px;
      background: #d3d3d3;
      outline: none;
      opacity: 0.7;
      -webkit-transition: .2s;
      transition: opacity .2s;
    }

    .slider:hover {
      opacity: 1;
    }
  
    .slider::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;
      width: 40px;
      height: 40px;
      border-radius: 50%;
      background: red;
      cursor: pointer;
    }

    .slider::-moz-range-thumb {
      width: 40px;
      height: 40px;
      border-radius: 50%;
      background: red;
      cursor: pointer;
    }    
    </style>



  </head>


  <body>
  <h1 style="color: teal; text-align: center;">ESP32 Web Control Car</h1>
<h2 style="text-align: center">WiFi Controller<h2>
<table id="mainTable" border="1" style="width:400px; margin:auto;table-layout:fixed" CELLSPACING=10>
  <tr>
    <td></td>
    <td class="button" ontouchstart='sendButtonInput("MoveCar","1")' ontouchend='sendButtonInput("MoveCar","0")'><span class="arrows">&#8679;</span></td>
    <td></td>
  </tr>
  
  <tr>
     <td class="button" ontouchstart='sendButtonInput("MoveCar","3")' ontouchend='sendButtonInput("MoveCar","0")'><span class="arrows">&#8678;</span></td>
  <td class="button"</td>   
     <td class="button" ontouchstart='sendButtonInput("MoveCar","4")' ontouchend='sendButtonInput("MoveCar","0")'><span class="arrows">&#8680;</span></td>
   </tr>  
  
   <tr>
    <td></td>
    <td class="button" ontouchstart='sendButtonInput("MoveCar","2")' ontouchend='sendButtonInput("MoveCar","0")'><span class="arrows">&#8681;</span></td>
    <td></td>
   </tr>
 
  <tr></tr>
  <tr></tr>
  <tr>
    <td style="text-align:center;font-size:24px"><b>Speed:</b></td>
    <td colspan=2> <!兩欄合併>
      <div class="slidecontainer">
        <input id="Speed" type="range" min="0" max="255" value="125" class="slider" oninput='sendButtonInput("Speed",value)'>
      </div>
    </td>
  </tr>
 </table>
    
  <script>
  var WebSocketCarInputUrl="ws:\/\/"+window.location.hostname+"/CarInput";
var WebSocketCarInput;

function  sendButtonInput(key,value)
{
  var data  = key + "," + value; // Speed,120; MoveCar,1
  WebSocketCarInput.send(data);
}


function  initCarInputWebSocket()
{
  WebSocketCarInput = new WebSocket(WebSocketCarInputUrl);
  WebSocketCarInput.onopen  = function(event)
  {
    var speedButton = documnet.getElementById("Speed");
    sendButtonInput("Speed",speedButton.value);
  };
  WebSocketCarInput.onclose = function(event)  {setTimeout(initCarInputWebSocket, 2000)};
  WebSocketCarInput.onmessage = function(event){};

}

window.onload=initCarInputWebSocket;
document..getElementById("mainTable").addEventListener("touchend",function(event){
  event.prentDefaut();
});  
  
  </script>
  
  </body>
</html>
)HTMLHOMEPAGE";

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

  request->send(200,"text/html",htmlHomePage);

}

void  handleNotFound(AsyncWebServerRequest *request)
{

  request->send(404,"text/plain","File Not Found");

}

void  rotateMotor(int motorNum, int motorDirection)
{
  if(motorDirection==FORWARD)
  {
    digitalWrite(motor_pins[motorNum].pinIN1,HIGH);
    digitalWrite(motor_pins[motorNum].pinIN2,LOW);
  }
  
  else if(motorDirection==BACKWARD)
  {
    digitalWrite(motor_pins[motorNum].pinIN1,LOW);
    digitalWrite(motor_pins[motorNum].pinIN2,HIGH);
  }
  else if(motorDirection==STOP)
  {
    digitalWrite(motor_pins[motorNum].pinIN1,LOW);
    digitalWrite(motor_pins[motorNum].pinIN2,LOW);
  }

}

void  moveCar(int valueInt)
{
  Serial.printf("Got value as %d\n", valueInt);
  switch (valueInt)
  {
  case UP/* constant-expression */:
    /* code */
    rotateMotor(RIGHT_MOTOR,FORWARD);
    rotateMotor(LEFT_MOTER,FORWARD);
   break;
  case DOWN/* constant-expression */:
    /* code */
    rotateMotor(RIGHT_MOTOR,BACKWARD);
    rotateMotor(LEFT_MOTER,BACKWARD);
   break; 

  case LEFT/* constant-expression */:
    /* code */
    rotateMotor(RIGHT_MOTOR,FORWARD);
    rotateMotor(LEFT_MOTER,BACKWARD);
    
    break;  
  case RIGHT/* constant-expression */:
    /* code */
    rotateMotor(RIGHT_MOTOR,BACKWARD);
    rotateMotor(LEFT_MOTER,FORWARD);   
  break;

  case STOP/* constant-expression */:
    /* code */
    rotateMotor(RIGHT_MOTOR,STOP);
    rotateMotor(LEFT_MOTER,STOP);  
    break;  
  
  default:
    break;
  }
}

void  onCarInputWebSocketEvent(AsyncWebSocket *server,AsyncWebSocketClient  *client,
                                AwsEventType type,
                                void *arg,
                                uint8_t *data,
                                size_t len)
{
  switch (type)
  {
    case WS_EVT_CONNECT:  /* constant-expression */
    /* code */
    Serial.printf("WebSocket client #%u connected from %s\n",client->id(), client->remoteIP().toString().c_str());
    break;

    case WS_EVT_DISCONNECT:  /* constant-expression */
    /* code */
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    break;

    case WS_EVT_DATA: /* constant-expression */
    /* code */
      AwsFrameInfo *info;
      info  = (AwsFrameInfo*)arg;
      if(info->final && info->index==0 && info->len==len && info->opcode==WS_TEXT)
      {
        std::string myData="";
        myData.assign((char *)data,len);
        std::istringstream ss(myData);
        std::string key,value;
        std::getline(ss,key,',');
        std::getline(ss, value,',');
        Serial.printf("Key [%s] Value [%s]\n", key, value); //Speed,120; MoveCar,1
        int valueInt  = atoi(value.c_str());
        if(key =="MoveCar")
        {
          moveCar(valueInt);
          
        }
        else  if(key=="Speed")
        {
          ledcWrite(PWMSpeedChannel,valueInt);
        }
      }
    break;

    case WS_EVT_PONG:  /* constant-expression */
    /* code */
    break;
    case WS_EVT_ERROR: /* constant-expression */
    /* code */
    break;
  
    default:
      break;
  }


}

void setup() {
  // put your setup code here, to run once:
  setUpPinModes();  //self function
  Serial.begin(11520);
  WiFi.softAP(ssid,password);
  IPAddress IP= WiFi.softAPIP();
  Serial1.print("AP IP address");
  Serial1.println(IP);

  server.on("/",HTTP_GET,handleRoot);
  server.onNotFound(handleNotFound);

  wsCarInput.onEvent(onCarInputWebSocketEvent);
  server.addHandler(&wsCarInput); // need to know memory address
  server.begin();
  Serial.println("HTTP Server started");




}

void loop() {
  // put your main code here, to run repeatedly:
  wsCarInput.cleanupClients();
}