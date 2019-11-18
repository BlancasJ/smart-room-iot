#include <ESP8266WiFi.h> // In case of using an ESP8266
#include <WiFiClient.h>    // In case of using an ESP32
#include <WiFi.h>          // In case of using an ESP32
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

/* Define the pins for the variables and actuator we will use */
  
// Led
#define led_pin 2

// Pulse
#define AC_pin 23
volatile byte dim = 0; //Initial brightness level from 0 to 255, change as you like!


/****************************************************************/

/* WIFI parameters */

#define WLAN_SSID "YOUR_WIFI_SSID" //      // change this to your ssid wifi
#define WLAN_PASS "YOUR_WIFI_PASSWORD" //    // change this to your password wifi

/* Adafruit IO */

#define AIO_SERVER  "soldier.cloudmqtt.com" //change this to your mqtt server ip
#define AIO_SERVERPORT  13857               // change this to your mqtt port
#define AIO_USERNAME  "YOUR_AIO_USERNAME"            // change this to your adafruit username
#define AIO_KEY "YOUR_AIO_KEY"              // change this to your AIO_KEY  

// Create an ESP8266 WIFIClient class to connect to the MQTT server

WiFiClient client;

// Store the MQTT server, client ID, username, and password in flash memory

const char MQTT_SERVER[] PROGMEM = AIO_SERVER;

// Set a unique MQTT client ID using the AIO key + the date and time the sketch
// was compiled (so this should be unique across multiple devices for the user,
// alternatively you can manually set this to a GUID or other random value)

const char MQTT_CLIENTID[] PROGMEM = AIO_KEY __DATE__ __TIME__;
const char MQTT_USERNAME[] PROGMEM = AIO_USERNAME;
const char MQTT_PASSWORD[] PROGMEM = AIO_KEY;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details

Adafruit_MQTT_Client mqtt(&client,MQTT_SERVER,AIO_SERVERPORT,MQTT_CLIENTID,MQTT_USERNAME,MQTT_PASSWORD);

/********************** Feeds **********************************************/

// Setup feeds for the fields 

const char LED_FEED[]PROGMEM = AIO_USERNAME "/feeds/led";
Adafruit_MQTT_Subscribe led_one = Adafruit_MQTT_Subscribe(&mqtt,LED_FEED);

const char DIMER_FEED[]PROGMEM = AIO_USERNAME "/feeds/dimer";
Adafruit_MQTT_Subscribe dimer_one = Adafruit_MQTT_Subscribe(&mqtt,DIMER_FEED);


/******************** Sketch Code ***********************************************/
/* Functions */

void connect();                       // function to connect to mqtt database
void TaskMQTT( void *pvParameters );  // define task for one core
void TaskDimer( void *pvParameters ); // define task for one core

TaskHandle_t Task1;
TaskHandle_t Task2;

// the setup function runs once when you press reset or power the board
void setup() 
{
  // initialize serial communication at 115200 bits per second:
  Serial.begin(115200);
  
  // Now set up two tasks to run independently.
  xTaskCreatePinnedToCore(
    TaskMQTT
    ,  "TaskMQTT"   // A name just for humans
    ,  10000  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  1  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  &Task1 
    ,  1);
  delay(500);
  xTaskCreatePinnedToCore(
    TaskDimer
    ,  "TaskDimer"
    ,  10000  // Stack size
    ,  NULL
    ,  1  // Priority
    ,  &Task2
    ,  0);
  delay(500);
  // Now the task scheduler, which takes over control of scheduling individual tasks, is automatically started.

  Serial.println(F("Adafruit IO Example"));

  // Connect to WiFi access point

  Serial.println(); Serial.println();
  delay(10);
  Serial.print(F("Connecting to "));
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID,WLAN_PASS);
  while(WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println();

  Serial.println(F("WiFi connected"));
  Serial.println(F("IP address:"));

  Serial.println(WiFi.localIP());

  mqtt.subscribe(&led_one);
  mqtt.subscribe(&dimer_one);
  
  // connect to adafruit io

  connect();
  
}

void loop()
{
  // Disable the watch dog timer for both cores
  disableCore0WDT();
  disableCore1WDT();
}

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/

void TaskMQTT(void *pvParameters)  // This is a task.
{
  (void) pvParameters;

  // initialize digital 
  
  // Led
  pinMode(led_pin,OUTPUT);

  for (;;) // A Task shall never return or exit.
  {
    // ping adafruit io a few times to make sure we remain connected
    if(!mqtt.ping(3))
    {
      // reconnect to adafruit to
      if(!mqtt.connected())
  
      connect();
    }
  
    // Subscribe data
    Adafruit_MQTT_Subscribe *subscription;
    while((subscription = mqtt.readSubscription(5000))) 
    {
      // Check if its the led_one button feed
      if (subscription == &led_one) 
      {
        Serial.print(F("On-Off button: "));
        Serial.println((char *)led_one.lastread);
        
        if (strcmp((char *)led_one.lastread, "0") == 0) 
        {
          digitalWrite(led_pin, LOW); 
        }
        if (strcmp((char *)led_one.lastread, "1") == 0) 
        {
          digitalWrite(led_pin, HIGH); 
        }
      }
      else if(subscription == &dimer_one)
      {
        Serial.print(F("Dimer: "));
        char * string_ = "";
        string_ = (char *)dimer_one.lastread;
        Serial.println(string_);
        dim = atoi(string_);
        
      }
    }
    vTaskDelay(100);  // one tick delay (15ms) in between reads for stability
  }
}

void TaskDimer(void *pvParameters)  // This is a task.
{
  (void) pvParameters;

  // Pulse
  pinMode(AC_pin, OUTPUT);
  // Lightness variable
  int val=0;
  
  for (;;)
  {
    if(digitalRead(22) == 1)
    {
      if (dim < 1 || dim > 100) 
      {      
        //Turn TRIAC completely OFF if dim is 0 or dim is 254
        digitalWrite(AC_pin, LOW);
      }
    
      if (dim > 0 && dim <= 100) 
      {
        //Dimming part, if dim is not 0 and not 255
        val = 32.0*(255.0-(dim*(255.0/100.0)));
        delayMicroseconds(val);
        digitalWrite(AC_pin, HIGH);
        delayMicroseconds(100);
        digitalWrite(AC_pin, LOW);
      }
    }
  }
}

// connect to adafruit io via MQTT
void connect()
{
  Serial.print(F("Connecting to Adafruit IO...."));
  int8_t ret;
  while((ret=mqtt.connect())!=0)
  {
    switch(ret)
    {
      case 1: Serial.println(F("Wrong protocol")); break;
      case 2: Serial.println(F("ID rejected")); break;
      case 3: Serial.println(F("Server unavail")); break;
      case 4: Serial.println(F("Bad user/pass")); break;
      case 5: Serial.println(F("Not authed")); break;
      case 6 : Serial.println(F("Failed to subscribe")); break;
      default: Serial.println(F("Connection failed")); break;
    }
    if(ret>=0)
    {
      mqtt.disconnect();
    }
    Serial.println(F("Retrying connection...."));
    delay(5000);
  }
  Serial.println(F("Adafruit IO Connected!")); 
}
