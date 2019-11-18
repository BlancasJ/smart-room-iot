// Smart Room IoT Controller — ESP32 Main Board
// ESP32 + XBee (RF) + DHT11 + MQ-2 + Relay + MQTT
// Team: Josmar Villanueva, Jorge Blancas, Eleser Dominguez,
//       Freddy Barredos, Andres Moguel
// Universidad Politecnica de Yucatan - 2019
//
// Architecture:
// - ESP32 connects to WiFi and MQTT broker (CloudMQTT)
// - Mobile app sends commands via MQTT (LED, valve, fan, dimmer levels)
// - ESP32 relays dimmer commands to PIC16F88 boards via XBee RF (Serial2)
// - PIC dimmers receive 4-char command: [ID_msb][ID_lsb][tens][units]
//   Example: "1650" = device ID 16, dimmer value 50%
// - ESP32 reads DHT11 (temp/humidity) and MQ-2 (gas) and publishes to MQTT
//
// Reconstructed from project photos, app screenshots, and PIC dimmer protocol.

#include <WiFi.h>
#include <WiFiClient.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "DHT.h"

// --- Pin Definitions ---
#define LED_PIN        2
#define VALVE_PIN      4
#define FAN_RELAY_PIN  5
#define DHT_PIN        15
#define MQ2_PIN        34
#define DHTTYPE        DHT11

// XBee RF communication via Serial2 (UART2)
#define XBEE_RX 16
#define XBEE_TX 17

// --- WiFi ---
#define WLAN_SSID "NETWORK_NAME"
#define WLAN_PASS "PASSWORD"

// --- MQTT ---
#define MQTT_SERVER     "MQTT_SERVER_ADDRESS"
#define MQTT_SERVERPORT 13857
#define MQTT_USERNAME   "MQTT_USERNAME"
#define MQTT_KEY        "MQTT_KEY"

// --- Dimmer Device IDs ---
#define BLUE_LIGHT_ID   16
#define GREEN_LIGHT_ID  17

// --- Objects ---
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_SERVERPORT, MQTT_KEY __DATE__ __TIME__, MQTT_USERNAME, MQTT_KEY);
DHT dht(DHT_PIN, DHTTYPE);

// --- MQTT Subscriptions (receive commands from app) ---
Adafruit_MQTT_Subscribe led_feed = Adafruit_MQTT_Subscribe(&mqtt, MQTT_USERNAME "/feeds/led");
Adafruit_MQTT_Subscribe valve_feed = Adafruit_MQTT_Subscribe(&mqtt, MQTT_USERNAME "/feeds/valve");
Adafruit_MQTT_Subscribe fan_feed = Adafruit_MQTT_Subscribe(&mqtt, MQTT_USERNAME "/feeds/fan");
Adafruit_MQTT_Subscribe blue_light_feed = Adafruit_MQTT_Subscribe(&mqtt, MQTT_USERNAME "/feeds/bluelight");
Adafruit_MQTT_Subscribe green_light_feed = Adafruit_MQTT_Subscribe(&mqtt, MQTT_USERNAME "/feeds/greenlight");

// --- MQTT Publishes (send sensor data to app) ---
Adafruit_MQTT_Publish temperature_pub = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME "/feeds/temperature");
Adafruit_MQTT_Publish humidity_pub = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME "/feeds/humidity");
Adafruit_MQTT_Publish gas_pub = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME "/feeds/gas");

// --- Timing ---
unsigned long last_sensor_read = 0;
#define SENSOR_INTERVAL 5000

// --- Functions ---
void connect_mqtt();
void send_dimmer_command(uint8_t device_id, uint8_t value);

void setup()
{
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, XBEE_RX, XBEE_TX);

  pinMode(LED_PIN, OUTPUT);
  pinMode(VALVE_PIN, OUTPUT);
  pinMode(FAN_RELAY_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(VALVE_PIN, LOW);
  digitalWrite(FAN_RELAY_PIN, LOW);

  dht.begin();

  // Connect to WiFi
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());

  // Subscribe to MQTT feeds
  mqtt.subscribe(&led_feed);
  mqtt.subscribe(&valve_feed);
  mqtt.subscribe(&fan_feed);
  mqtt.subscribe(&blue_light_feed);
  mqtt.subscribe(&green_light_feed);

  connect_mqtt();
}

void loop()
{
  // Ensure MQTT connection
  if (!mqtt.ping(3))
  {
    if (!mqtt.connected())
      connect_mqtt();
  }

  // Process incoming MQTT messages
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(1000)))
  {
    // LED toggle
    if (subscription == &led_feed)
    {
      String val = (char *)led_feed.lastread;
      Serial.print("LED: ");
      Serial.println(val);
      if (val == "1")
        digitalWrite(LED_PIN, HIGH);
      else
        digitalWrite(LED_PIN, LOW);
    }

    // Valve toggle
    if (subscription == &valve_feed)
    {
      String val = (char *)valve_feed.lastread;
      Serial.print("Valve: ");
      Serial.println(val);
      if (val == "1")
        digitalWrite(VALVE_PIN, HIGH);
      else
        digitalWrite(VALVE_PIN, LOW);
    }

    // Fan relay toggle
    if (subscription == &fan_feed)
    {
      String val = (char *)fan_feed.lastread;
      Serial.print("Fan: ");
      Serial.println(val);
      if (val == "1")
        digitalWrite(FAN_RELAY_PIN, HIGH);
      else
        digitalWrite(FAN_RELAY_PIN, LOW);
    }

    // Blue light dimmer (0-100) → send via XBee to PIC dimmer
    if (subscription == &blue_light_feed)
    {
      uint8_t dim_value = atoi((char *)blue_light_feed.lastread);
      Serial.print("Blue Light: ");
      Serial.println(dim_value);
      send_dimmer_command(BLUE_LIGHT_ID, dim_value);
    }

    // Green light dimmer (0-100) → send via XBee to PIC dimmer
    if (subscription == &green_light_feed)
    {
      uint8_t dim_value = atoi((char *)green_light_feed.lastread);
      Serial.print("Green Light: ");
      Serial.println(dim_value);
      send_dimmer_command(GREEN_LIGHT_ID, dim_value);
    }
  }

  // Read sensors periodically
  if (millis() - last_sensor_read > SENSOR_INTERVAL)
  {
    last_sensor_read = millis();

    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    int gas_raw = analogRead(MQ2_PIN);
    float gas_level = gas_raw / 4095.0;

    if (!isnan(temperature))
    {
      Serial.print("Temp: ");
      Serial.print(temperature);
      Serial.print("C  Hum: ");
      Serial.print(humidity);
      Serial.print("%  Gas: ");
      Serial.println(gas_level);

      temperature_pub.publish(temperature);
      humidity_pub.publish(humidity);
      gas_pub.publish(gas_level);
    }
  }
}

// Send dimmer command to PIC16F88 via XBee RF (Serial2)
// Protocol: 4 chars → [ID_tens][ID_units][value_tens][value_units]
// Example: device_id=16, value=50 → sends "1650"
void send_dimmer_command(uint8_t device_id, uint8_t value)
{
  if (value > 100) value = 100;

  char cmd[5];
  cmd[0] = '0' + (device_id / 10);
  cmd[1] = '0' + (device_id % 10);
  cmd[2] = '0' + (value / 10);
  cmd[3] = '0' + (value % 10);
  cmd[4] = '\0';

  Serial2.print(cmd);
  Serial.print("XBee TX: ");
  Serial.println(cmd);
}

void connect_mqtt()
{
  Serial.print("Connecting to MQTT...");
  int8_t ret;
  while ((ret = mqtt.connect()) != 0)
  {
    switch (ret)
    {
      case 1: Serial.println("Wrong protocol"); break;
      case 2: Serial.println("ID rejected"); break;
      case 3: Serial.println("Server unavailable"); break;
      case 4: Serial.println("Bad user/pass"); break;
      case 5: Serial.println("Not authorized"); break;
      case 6: Serial.println("Failed to subscribe"); break;
      default: Serial.println("Connection failed"); break;
    }
    if (ret >= 0)
      mqtt.disconnect();
    Serial.println("Retrying...");
    delay(5000);
  }
  Serial.println("MQTT Connected!");
}
