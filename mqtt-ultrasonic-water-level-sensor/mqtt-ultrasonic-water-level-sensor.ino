#include "credentials.h";

#include <SoftwareSerial.h>;
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define US100_TX        15 // Connected to US100 TX Pin
#define US100_RX        13 // Connected to US100 RX Pin

#define TPL_DONE        12 // Connected to DONE pin of TPL5110 power timer

#define MAX_UPTIME      10000 // Maximum uptime of 10s

#define NTP_SERVER_0    "pool.ntp.org"
#define NTP_SERVER_1    "time.nist.gov"

#define MQTT_SERVER     "broker.mqtt-dashboard.com"
#define MQTT_PORT       1883
#define MQTT_CLIENT     "esp8266_sensor1"
#define MQTT_OUT_TOPIC  "moritzmhmk/water"

unsigned long startTime;
SoftwareSerial US100Serial(US100_RX, US100_TX);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void setup() {
  // setup pin for external timer
  digitalWrite(TPL_DONE, LOW);
  pinMode(TPL_DONE, OUTPUT);
    
  startTime = millis();
  Serial.begin(9600);
  US100Serial.begin(9600);
  Serial.println("Starting.");
  
  // Setup WiFi
  WiFi.persistent(false);
  WiFi.setAutoConnect(false);
  WiFi.setAutoReconnect(true);

  // Setup NTP
  configTime(0, 0, NTP_SERVER_0, NTP_SERVER_1);

  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
}

void loop() {
  if (millis() - startTime > MAX_UPTIME) {
    turn_power_off();
  }
  Serial.println("# LOOP");
  if (WiFi.status() != WL_CONNECTED) {
    Serial.printf("Connecting to WLAN \"%s\" ...", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.println("failed!");
      return;
    }
    Serial.println("done.");
    Serial.print("IP address: "); Serial.println(WiFi.localIP());
  }
  
  
  if (!mqttClient.connected()) { 
    Serial.printf("Connecting to mqtt broker \"%s:%d\" as client \"%s\"...", MQTT_SERVER, MQTT_PORT, MQTT_CLIENT);
    if (mqttClient.connect(MQTT_CLIENT)) {
      Serial.println("done.");
    } else {
      Serial.printf("failed (rc=%s).", mqttClient.state());
      return;
    }
  }


  time_t now = time(nullptr);
  if (now < (2020 - 1970) * 365 * 24 * 3600) { // no timetravel before 2020 with this device...
    Serial.println("Device time is not synced (yet).");
    return;
  }
  

  Serial.print("Get distance...");
  unsigned int distance = measure_distance(US100Serial);
  if (distance > 4500) { // max sensing distance is 450cm (4500mm), on error returns 11209
    Serial.printf("failed (distance of %u mm is out of range).\n", distance);
    return;
  }
  Serial.printf("done (%u mm).\n", distance);


  Serial.printf("Publishing on mqtt in topic %s.\n", MQTT_OUT_TOPIC);
  char msg[255];
  sprintf(msg, "{\"timestamp\": %u, \"distance\": %u}", now, distance);
  Serial.print("Message: "); Serial.println(msg);
  mqttClient.publish(MQTT_OUT_TOPIC, msg);
  mqttClient.loop();


  turn_power_off();
}

unsigned int measure_distance(Stream &sensorSerial) {
  sensorSerial.flush();
  sensorSerial.write(0x55); 
  delay(100); 
  unsigned int msb = sensorSerial.read(); 
  unsigned int lsb = sensorSerial.read();
  unsigned int distance  = msb * 256 + lsb; 
  return distance;
}

void turn_power_off() {
  Serial.printf("Set TPL Power Timer DONE Pin (uptime was %d ms).\n", millis() - startTime);
  while (1) {
    digitalWrite(TPL_DONE, HIGH);
    delay(1);
    digitalWrite(TPL_DONE, LOW);
    delay(1);
  }
}
