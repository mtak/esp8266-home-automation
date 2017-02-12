/*
 * This sketch polls an LDR to watch for pulses
 * of a digital power meter. 
 * Blog post at: http://mtak.nl/it/esp8266-based-power-meter-in-kibana/
 *
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include "DHT.h"
#include <TM1637Display.h>

#define DHTPIN D4  
#define DHTTYPE DHT11

#define SEGCLK D5
#define SEGDIO D6

const char* ssid = "Clients";
const char* password = "password";
const char* mqtt_server = "dom1.int.mtak.nl";

const char* dhtTopic = "meterkast/dht";
const char* ldrTopic = "meterkast/power";

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);
TM1637Display display(SEGCLK, SEGDIO);

long lastDht = 0;
char msg[50];
long lastLdrTime = 0;
long lastLdrUpdate = 0;
long lastDisplayUpdate = 0;
int lastDisplayState = 0;
int lastPower = 0;
int lastTemp = 0;

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  digitalWrite(BUILTIN_LED ,0);     // Turn LED on while starting

  uint8_t data[] = { 0x38, 0x3f, 0x77, 0x5e };  // LOAd
  display.setBrightness(0x0a);
  display.setSegments(data);
  
  Serial.begin(115200);
  setup_wifi();
  otaSetup();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  dht.begin();
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
  ArduinoOTA.setHostname("esp2");

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

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("esp2", "esp2", "auKae7yei7Achah")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      // client.publish("outTopic", "hello world");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void sendDhtSensor() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    client.publish("dhtTopic", "Failed to read from DHT sensor");
    return;
  }

  char dhtMessage[120];
  sprintf(dhtMessage, "{ \"temperature\": %d, \"humidity\": %d }", round(t), round(h));
  lastTemp = round(t);

  Serial.print("Publish message: ");
  Serial.println(dhtMessage);
  
  client.publish(dhtTopic, dhtMessage);
}

void sendPower(long ldrTime) {
  float watts;

  watts = 3600000 / ldrTime;
  
  char ldrMessage[120];
  sprintf(ldrMessage, "{ \"power\": %d }", round(watts) );
  lastPower = round(watts);
  
  Serial.print("Publish message: ");
  Serial.println(ldrMessage);
  
  client.publish(ldrTopic, ldrMessage);
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
  if (now - lastDht > 60000) {
    lastDht = now;
    sendDhtSensor();
  }

  // Only read every 20ms
  //currentTime = millis();
  if ( lastLdrUpdate < ( now - 20 ) ) {
    lastLdrUpdate = now;
    
    // Send power data when necessary (threshold = 200)
    long ldrValue;
    ldrValue = analogRead(A0);
    if ( ldrValue > 200 ) {
      long ldrNow = millis();
    
      long ldrTime;
      ldrTime = ldrNow - lastLdrTime;
    
      // Skip double detections
      if ( ldrTime < 500 ) { return; }
    
      lastLdrTime = ldrNow;
      sendPower(ldrTime);
    }

  }  // lastLdrUpdate < 20ms 


  // Update TM1637 4-segment display
  if (now - lastDisplayUpdate > 5000) {
    lastDisplayUpdate = now;
    if ( lastDisplayState == 0 ) {
      Serial.println("Displaying power");
      lastDisplayState = 1;
      display.showNumberDec(lastPower,false);
    } else {
      Serial.println("Displaying temp");
      lastDisplayState = 0;
      display.showNumberDec(lastTemp,false);
    }
  }

}

