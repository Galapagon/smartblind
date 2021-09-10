#include <SimpleTimer.h>    //https://github.com/marcelloromani/Arduino-SimpleTimer/tree/master/SimpleTimer
#include <ESP8266WiFi.h>    //if you get an error here you need to install the ESP8266 board manager 
#include <ESP8266mDNS.h>    //if you get an error here you need to install the ESP8266 board manager 
#include <PubSubClient.h>   //https://github.com/knolleary/pubsubclient
#include <ArduinoOTA.h>     //https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA
#include <AH_EasyDriver.h>  //http://www.alhin.de/arduino/downloads/AH_EasyDriver_20120512.zip

/*****************  START USER CONFIG SECTION *********************************/
/*****************  START USER CONFIG SECTION *********************************/
/*****************  START USER CONFIG SECTION *********************************/
/*****************  START USER CONFIG SECTION *********************************/

#define USER_SSID                 "YOUR_SSID"
#define USER_PASSWORD             "YOUR_WIFI_PASSWORD"
#define USER_MQTT_SERVER          "YOUR_MQTT_SERVER_ADDRESS"
#define USER_MQTT_PORT            1883
#define USER_MQTT_USERNAME        "YOUR_MQTT_USER_NAME"
#define USER_MQTT_PASSWORD        "YOUR_MQTT_PASSWORD"
#define USER_MQTT_CLIENT_NAME     "BlindsMCU"         // Used to define MQTT topics, MQTT Client ID, and ArduinoOTA

#define STEPPER_SPEED             35                  //Defines the speed in RPM for your stepper motor
#define STEPPER_STEPS_PER_REV     1028                //Defines the number of pulses that is required for the stepper to rotate 360 degrees
#define STEPPER_MICROSTEPPING     0                   //Defines microstepping 0 = no microstepping, 1 = 1/2 stepping, 2 = 1/4 stepping 
#define DRIVER_INVERTED_SLEEP     1                   //Defines sleep while pin high.  If your motor will not rotate freely when on boot, comment this line out.

#define STEPS_TO_CLOSE            18                  //Defines the number of steps needed to open or close fully

#define STEPPER_DIR_PIN           D6
#define STEPPER_STEP_PIN          D7
#define STEPPER_SLEEP_PIN         D5
#define STEPPER_DIR_PIN2           D1
#define STEPPER_STEP_PIN2          D3
#define STEPPER_SLEEP_PIN2         D2
#define STEPPER_MICROSTEP_1_PIN   14
#define STEPPER_MICROSTEP_2_PIN   12

/*****************  END USER CONFIG SECTION *********************************/
/*****************  END USER CONFIG SECTION *********************************/
/*****************  END USER CONFIG SECTION *********************************/
/*****************  END USER CONFIG SECTION *********************************/
/*****************  END USER CONFIG SECTION *********************************/

WiFiClient espClient;
PubSubClient client(espClient);
SimpleTimer timer;
AH_EasyDriver shadeStepper(STEPPER_STEPS_PER_REV, STEPPER_DIR_PIN ,STEPPER_STEP_PIN,STEPPER_MICROSTEP_1_PIN,STEPPER_MICROSTEP_2_PIN,STEPPER_SLEEP_PIN);
AH_EasyDriver shadeStepper2(STEPPER_STEPS_PER_REV, STEPPER_DIR_PIN2 ,STEPPER_STEP_PIN2,STEPPER_MICROSTEP_1_PIN,STEPPER_MICROSTEP_2_PIN,STEPPER_SLEEP_PIN2);

//Global Variables
bool boot = true;
int currentPosition = 0;
int currentPosition2 = 0;
int newPosition = 0;
int newPosition2 = 0;
char positionPublish[50];
bool moving = false;
bool moving2 = false;
char charPayload[50];

const char* ssid = USER_SSID ; 
const char* password = USER_PASSWORD ;
const char* mqtt_server = USER_MQTT_SERVER ;
const int mqtt_port = USER_MQTT_PORT ;
const char *mqtt_user = USER_MQTT_USERNAME ;
const char *mqtt_pass = USER_MQTT_PASSWORD ;
const char *mqtt_client_name = USER_MQTT_CLIENT_NAME ; 




//Functions
void setup_wifi() {
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() 
{
  int retries = 0;
  while (!client.connected()) {
    if(retries < 150)
    {
      Serial.print("Attempting MQTT connection...");
      if (client.connect(mqtt_client_name, mqtt_user, mqtt_pass)) 
      {
        Serial.println("connected");
        if(boot == false)
        {
          client.publish(USER_MQTT_CLIENT_NAME"/MasterBedroom/checkIn","Reconnected"); 
        }
        if(boot == true)
        {
          client.publish(USER_MQTT_CLIENT_NAME"/MasterBedroom/checkIn","Rebooted");
        }
        // ... and resubscribe
        client.subscribe(USER_MQTT_CLIENT_NAME"MasterBedroom/Right/blindsCommand");
        client.subscribe(USER_MQTT_CLIENT_NAME"MasterBedroom/Right/positionCommand");
        client.subscribe(USER_MQTT_CLIENT_NAME"MasterBedroom/Left/blindsCommand");
        client.subscribe(USER_MQTT_CLIENT_NAME"MasterBedroom/Left/positionCommand");
      } 
      else 
      {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        retries++;
        // Wait 5 seconds before retrying
        delay(5000);
      }
    }
    if(retries > 149)
    {
    ESP.restart();
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Message arrived [");
  String newTopic = topic;
  Serial.print(topic);
  Serial.print("] ");
  payload[length] = '\0';
  String newPayload = String((char *)payload);
  int intPayload = newPayload.toInt();
  Serial.println(newPayload);
  Serial.println();
  newPayload.toCharArray(charPayload, newPayload.length() + 1);
  if (newTopic == USER_MQTT_CLIENT_NAME"MasterBedroom/Right/blindsCommand") 
  {
    if (newPayload == "OPEN")
    {
      client.publish(USER_MQTT_CLIENT_NAME"MasterBedroom/Right/positionCommand", "0", true);
    }
    else if (newPayload == "CLOSE")
    {   
      int stepsToClose = STEPS_TO_CLOSE;
      String temp_str = String(stepsToClose);
      temp_str.toCharArray(charPayload, temp_str.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME"MasterBedroom/Right/positionCommand", charPayload, true);
    }
    else if (newPayload == "STOP")
    {
      String temp_str = String(currentPosition);
      temp_str.toCharArray(positionPublish, temp_str.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME"MasterBedroom/Right/positionCommand", positionPublish, true); 
    }
  }
  if (newTopic == USER_MQTT_CLIENT_NAME"MasterBedroom/Left/blindsCommand") 
  {
    if (newPayload == "OPEN")
    {
      client.publish(USER_MQTT_CLIENT_NAME"MasterBedroom/Left/positionCommand", "0", true);
    }
    else if (newPayload == "CLOSE")
    {   
      int stepsToClose = STEPS_TO_CLOSE;
      String temp_str = String(stepsToClose);
      temp_str.toCharArray(charPayload, temp_str.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME"MasterBedroom/Left/positionCommand", charPayload, true);
    }
    else if (newPayload == "STOP")
    {
      String temp_str = String(currentPosition);
      temp_str.toCharArray(positionPublish, temp_str.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME"MasterBedroom/Left/positionCommand", positionPublish, true); 
    }
  }
  if (newTopic == USER_MQTT_CLIENT_NAME"MasterBedroom/Right/positionCommand")
  {
    if(boot == true)
    {
      newPosition = intPayload;
      currentPosition = intPayload;
      boot = false;
    }
    if(boot == false)
    {
      newPosition = intPayload;
    }
  }
    if (newTopic == USER_MQTT_CLIENT_NAME"MasterBedroom/Left/positionCommand")
  {
    if(boot == true)
    {
      newPosition2 = intPayload;
      currentPosition2 = intPayload;
      boot = false;
    }
    if(boot == false)
    {
      newPosition2 = intPayload;
    }
  }
}

void processStepper()
{ 
  if (newPosition > currentPosition)  //Right
  {
    #if DRIVER_INVERTED_SLEEP == 1
    shadeStepper.sleepON();
    #endif
    #if DRIVER_INVERTED_SLEEP == 0
    shadeStepper.sleepOFF();
    #endif
    shadeStepper.move(80, FORWARD);
    currentPosition++;
    moving = true;
  }
  if (newPosition2 > currentPosition2)  //Left
  {
    #if DRIVER_INVERTED_SLEEP == 1
    shadeStepper2.sleepON();
    #endif
    #if DRIVER_INVERTED_SLEEP == 0
    shadeStepper2.sleepOFF();
    #endif
    shadeStepper2.move(80, FORWARD);
    currentPosition2++;
    moving2 = true;
  }
  if (newPosition < currentPosition) //Right
  {
    #if DRIVER_INVERTED_SLEEP == 1
    shadeStepper.sleepON();
    #endif
    #if DRIVER_INVERTED_SLEEP == 0
    shadeStepper.sleepOFF();
    #endif
    shadeStepper.move(80, BACKWARD);
    currentPosition--;
    moving = true;
  }
  if (newPosition2 < currentPosition2) //Left
  {
    #if DRIVER_INVERTED_SLEEP == 1
    shadeStepper2.sleepON();
    #endif
    #if DRIVER_INVERTED_SLEEP == 0
    shadeStepper2.sleepOFF();
    #endif
    shadeStepper2.move(80, BACKWARD);
    currentPosition2--;
    moving2 = true;
  }
  if (newPosition == currentPosition && moving == true) //Right
  {
    #if DRIVER_INVERTED_SLEEP == 1
    shadeStepper.sleepOFF();
    #endif
    #if DRIVER_INVERTED_SLEEP == 0
    shadeStepper.sleepON();
    #endif
    String temp_str = String(currentPosition);
    temp_str.toCharArray(positionPublish, temp_str.length() + 1);
    client.publish(USER_MQTT_CLIENT_NAME"MasterBedroom/Right/positionState", positionPublish); 
    moving = false;
  }
  if (newPosition2 == currentPosition2 && moving2 == true) //Left
  {
    #if DRIVER_INVERTED_SLEEP == 1
    shadeStepper2.sleepOFF();
    #endif
    #if DRIVER_INVERTED_SLEEP == 0
    shadeStepper2.sleepON();
    #endif
    String temp_str = String(currentPosition);
    temp_str.toCharArray(positionPublish, temp_str.length() + 1);
    client.publish(USER_MQTT_CLIENT_NAME"MasterBedroom/Left/positionState", positionPublish); 
    moving2 = false;
  }
  Serial.println(currentPosition);
  Serial.println(newPosition);
  Serial.println(currentPosition2);
  Serial.println(newPosition2);
}

void checkIn()
{
  client.publish(USER_MQTT_CLIENT_NAME"MasterBedroom/checkIn","OK"); 
}


//Run once setup
void setup() {
  Serial.begin(115200);
  shadeStepper.setMicrostepping(STEPPER_MICROSTEPPING);            // 0 -> Full Step                                
  shadeStepper.setSpeedRPM(STEPPER_SPEED);     // set speed in RPM, rotations per minute
  #if DRIVER_INVERTED_SLEEP == 1
  shadeStepper.sleepOFF();
  #endif
  #if DRIVER_INVERTED_SLEEP == 0
  shadeStepper.sleepON();
  #endif
  WiFi.mode(WIFI_STA);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  ArduinoOTA.setHostname(USER_MQTT_CLIENT_NAME);
  ArduinoOTA.begin(); 
  delay(10);
  timer.setInterval(((1 << STEPPER_MICROSTEPPING)*5800)/STEPPER_SPEED, processStepper);   
  timer.setInterval(90000, checkIn);
}

void loop() 
{
  if (!client.connected()) 
  {
    reconnect();
  }
  client.loop();
  ArduinoOTA.handle();
  timer.run();
}