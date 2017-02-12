/*
 * This sketch polls several DS18b20 temperature sensors
 * and submits the values to an MQTT topic
 *
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS D4
#define LED D3

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

const char* ssid = "Clients";
const char* password = "password";
const char* mqtt_server = "dom1.int.mtak.nl";

const char* tempTopic = "slaapkamer/temp";

WiFiClient espClient;
PubSubClient client(espClient);

int lastTemp = 0;

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  pinMode(LED, OUTPUT);
  digitalWrite(BUILTIN_LED ,0);     // Turn LED on while starting
  digitalWrite(LED, 0);
  
  Serial.begin(9600);
  setup_wifi();
  otaSetup();
  client.setServer(mqtt_server, 1883);

  sensors.begin();

  digitalWrite(BUILTIN_LED ,HIGH);     // Turn LED off
}

void setup_wifi() {

  delay(10);
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

void otaSetup() {
  Serial.println("otaSetup start");
  // Port defaults to 8266
  ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("esp4");

  // No authentication by default
  ArduinoOTA.setPassword((const char *)"password");

  ArduinoOTA.onStart([]() {
    Serial.println("ArduinoOTA update start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nArduinoOTA update end");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("ArduinoOTA update progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("ArdionoOTA update error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("otaSetup done");
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("esp4", "esp4", "PeeGoo4Pi4joh1A")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void sendTempSensor() {
  // digitalWrite(LED, 1);
  float temp = 0;
  
  sensors.requestTemperatures();
  temp = sensors.getTempCByIndex(0);

  char tempMessage[4];
  dtostrf(temp, 4, 1, tempMessage);

  Serial.print("Publish message: ");
  Serial.println(temp);
  
  client.publish(tempTopic, tempMessage);
  // delay(20);
  // digitalWrite(LED, 0);
}

void loop() {
  // OTA firmware updates
  ArduinoOTA.handle();

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();

  // Send DHT sensor data every 60s
  if (now - lastTemp > 60000) {
    lastTemp = now;
    sendTempSensor();
  }

}

