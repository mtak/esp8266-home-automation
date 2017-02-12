/*
 *  This sketch monitors MQTT topics and displays status and
 *  animations on an LED display.
 *  
 */

#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>

// Neopixel Ring
#define RINGPIN D5
#define RINGNUMPIXELS 12
#define RINGMAXINTENSITY 130
Adafruit_NeoPixel ring = Adafruit_NeoPixel(RINGNUMPIXELS, RINGPIN, NEO_GRB + NEO_KHZ800);

// Strip 1 (left)
#define S1PIN D6
#define S1NUMPIXELS 8
#define S1MAXINTENSITY 130
Adafruit_NeoPixel strip1 = Adafruit_NeoPixel(S1NUMPIXELS, S1PIN, NEO_GRB + NEO_KHZ800);

// Strip 2 (right)
#define S2PIN D7
#define S2NUMPIXELS 8
#define S2MAXINTENSITY 130
Adafruit_NeoPixel strip2 = Adafruit_NeoPixel(S2NUMPIXELS, S2PIN, NEO_GRB + NEO_KHZ800);

const char* ssid = "Clients";
const char* password = "password";

const uint32_t orange = ring.Color(237, 120, 6);
const uint32_t green  = ring.Color(0, 255, 0);
const uint32_t red    = ring.Color(255, 0, 0);

int woonKamerOn = 2;

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);

void setup() {
  ring.begin();
  strip1.begin();
  strip2.begin();
  ring.setBrightness(RINGMAXINTENSITY); // Set overall brightness
  strip1.setBrightness(S1MAXINTENSITY);
  strip2.setBrightness(S2MAXINTENSITY);
  
  setAllPixels(ring, orange);
  setAllPixels(strip1, green);
  setAllPixels(strip2, red);
  
  ring.show(); // Initialize all pixels to 'off'
  strip1.show();
  strip2.show();

  Serial.begin(115200);
  delay(10);

  // prepare GPIO0
  pinMode(0, OUTPUT);
  digitalWrite(0, 0);
  
  // Connect to WiFi network
  Serial.println();
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
  
  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());

  // End of startup sequence
  pinMode(0, OUTPUT);
  digitalWrite(0, HIGH); // On-board LED off
  allPixelsOff();
}

void loop() {
  ledUpkeep();
    
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  
  // Wait until the client sends some data
  Serial.println("new client");
  while(!client.available()){
    delay(1);
  }
  
  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();
  
  // Match the request
  int val;
  if (req.indexOf("/ring/off") != -1) {
    printWebResponse(client);
    client.stop();
    allPixelsOff();
  } 
  else if (req.indexOf("/ring/start") != -1) {
    printWebResponse(client);
    client.stop();
    woonKamerStart();
  }
  else if (req.indexOf("/ring/stop") != -1) {
    printWebResponse(client);
    client.stop();
    woonKamerStop();
  }
  else if (req.indexOf("/ring/bitcoin") != -1) {
    printWebResponse(client);
    client.stop();
    initBitcoinRing(req);
  }
  else if (req.indexOf("/ring/status/on") != -1) { // Set status without animation
    printWebResponse(client);
    client.stop();
    woonKamerOn = 1;
  }
  else if (req.indexOf("/ring/status/off") != -1) { // Set status without animation
    printWebResponse(client);
    client.stop();
    woonKamerOn = 0;
  }
  else if (req.indexOf("/ring/demo") != -1) {
    printWebResponse(client);
    client.stop();
    NeoPixelDemo();
  }
  else {
    Serial.println("invalid request");
    client.stop();
    return;
  }
  
}

void ledUpkeep() {
  woonKamerLedUpkeep();
}


//
// Bitcoin LED
//
int getBitcoinPercentageFromHttp(String req) {
  // GET /ring/bitcoin/11 HTTP/1.1
  // Find second to last / and last space
  int httpSlash = req.lastIndexOf('/');
  int secondSlash = req.lastIndexOf('/', httpSlash - 1);
  int lastSpace = req.lastIndexOf(' ');
  return req.substring(secondSlash + 1, lastSpace).toInt();
}

int bitcoinPercentage = 0;
void initBitcoinRing(String req) {
  bitcoinPercentage = getBitcoinPercentageFromHttp(req);

  setBitcoinDisplay();
}

void setBitcoinBackground() {
  uint32_t backgroundColor = ring.Color(15, 7, 0); // Orange
  if (bitcoinPercentage > 0) {
    backgroundColor = ring.Color(0, 5, 2); // Green
  } else {
    backgroundColor = ring.Color(5, 0, 0); // Red
  }
    
  for (int led = 3; led <= 9; led++) {
    ring.setPixelColor(led, backgroundColor);
    ring.show();
  }
}

void setBitcoinForeground() {
  uint32_t backgroundColor;
  if (bitcoinPercentage > 0) {
    backgroundColor = ring.Color(0, 70, 0); // Green
  } else {
    backgroundColor = ring.Color(70, 0, 0); // Red
  }

  int noPixels = abs(bitcoinPercentage / 2);
  if (noPixels > 7) { noPixels = 7; } // Not too much LED's active

  for (int led = 3; led <= 3 + noPixels - 1; led++) {
    ring.setPixelColor(led, backgroundColor);
    ring.show();
  }

}

void setBitcoinDisplay() {
  delay(200);
  setBitcoinBackground();
  setBitcoinForeground();
  delay(200);
}

//
// Woonkamer status LED
//
int woonKamerIntensity = 0;
int cycleCounter = 0;
void woonKamerLedUpkeep() {
  if ( cycleCounter < 2000 ) {
    cycleCounter++;
    return;
  } else {
    cycleCounter=0;
  }

  int displayIntensity = int(100.0 + (65.0 * sin(woonKamerIntensity / 60.0)));

  if ( woonKamerIntensity < 376 ) {
    woonKamerIntensity++;
  } else {
    woonKamerIntensity = 0;
  }

  if ( woonKamerOn == 1 ) {
    ring.setPixelColor(0, ring.Color(0,displayIntensity,0)); // Green flash
    ring.show();
  } else if ( woonKamerOn == 0 ) { // woonKamer off
    ring.setPixelColor(0, ring.Color(displayIntensity,0,0)); // Red flash
    ring.show();
  } else {
    ring.setPixelColor(0, orange); // Orange flash
    ring.show();
  }
}


void woonKamerStart() {
  Serial.println("Begin woonKamerStart animation");
  woonKamerOn = 1;
  colorWipe(orange, 5000); //5000
  for(uint16_t i=0; i<ring.numPixels(); i++) {
    ring.setPixelColor(i, ring.Color(0, 255, 0));
  }
  ring.show();
  delay(500);
  //fadeAllPixelsOut(orange);
  allPixelsOff();
  woonKamerIntensity = 0;
  setBitcoinDisplay();
  Serial.println("End woonKamerStart animation");
  //rainbowCycle(1);
}

void woonKamerStop() {
  Serial.println("Begin woonKamerStop animation");
  woonKamerOn = 0;
  colorWipeReverse(ring.Color(RINGMAXINTENSITY, 0, 0), 300); //5000
  woonKamerIntensity = 0;
  allPixelsOff();
  setBitcoinDisplay();
  Serial.println("End woonKamerStop animation");
}


void printWebResponse(WiFiClient client) {
  client.flush();
  
  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nRunning...</html>\n";

  // Send the response to the client
  client.print(s);
  delay(1);
  Serial.println("Client disonnected");
  // The client will actually be disconnected 
  // when the function returns and 'client' object is detroyed
}

//
// Functions for Neopixel wheel
//
// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint32_t wait) {
  allPixelsOff();
  for(uint16_t i=0; i<ring.numPixels(); i++) {
    ring.setPixelColor(i, c);
    ring.show();
    delay(wait);
  }
}

void colorWipeReverse(uint32_t c, uint32_t wait) {
  allPixelsOff();
  for(uint16_t i=0; i<ring.numPixels(); i++) {
    ring.setPixelColor(i, c);
    ring.show();
  }
  for(int i=ring.numPixels(); i>=0; i--) {
    ring.setPixelColor(i, ring.Color(0,0,0));
    ring.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<ring.numPixels(); i++) {
      ring.setPixelColor(i, Wheel((i+j) & 255));
    }
    ring.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< ring.numPixels(); i++) {
      ring.setPixelColor(i, Wheel(((i * 256 / ring.numPixels()) + j) & 255));
    }
    ring.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < ring.numPixels(); i=i+3) {
        ring.setPixelColor(i+q, c);    //turn every third pixel on
      }
      ring.show();

      delay(wait);

      for (uint16_t i=0; i < ring.numPixels(); i=i+3) {
        ring.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < ring.numPixels(); i=i+3) {
        ring.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
      }
      ring.show();

      delay(wait);

      for (uint16_t i=0; i < ring.numPixels(); i=i+3) {
        ring.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return ring.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return ring.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return ring.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

// Turn all pixels off
void allPixelsOff() {
  for(int i=0;i<RINGNUMPIXELS;i++){

    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    ring.setPixelColor(i, ring.Color(0,0,0));

    strip1.setPixelColor(i, strip1.Color(0,0,0));
    strip2.setPixelColor(i, strip2.Color(0,0,0));

    ring.show(); // This sends the updated pixel color to the hardware.
    strip1.show();
    strip2.show();

  }
}

void setAllPixels(Adafruit_NeoPixel strip, uint32_t c) {
  for(int i=0;i<strip.numPixels();i++){
    strip.setPixelColor(i, c);
    strip.show();
  }
}

void fadeAllPixelsOut(uint32_t c) {
  uint8_t r = (uint8_t)(c >> 16);
  uint8_t g = (uint8_t)(c >>  8);
  uint8_t b = (uint8_t)c;

  for(int x=0;x<388;x++) {
    int rNew = int(-0.65 * x + r);
    int gNew = int(-0.65 * x + g);
    int bNew = int(-0.65 * x + b);

    for(int i=0;i<RINGNUMPIXELS;i++){
      ring.setPixelColor(i, ring.Color(rNew,gNew,bNew));
      ring.show();
    }
    delay(100);
  }
}

void NeoPixelDemo() {
  ring.setBrightness(255);

  // Some example procedures showing how to display to the pixels:
  colorWipe(ring.Color(255, 0, 0), 50); // Red
  colorWipe(ring.Color(0, 255, 0), 50); // Green
  colorWipe(ring.Color(0, 0, 255), 50); // Blue
  // Send a theater pixel chase in...
  theaterChase(ring.Color(127, 127, 127), 50); // White
  theaterChase(ring.Color(127, 0, 0), 50); // Red
  theaterChase(ring.Color(0, 0, 127), 50); // Blue

  rainbow(10);
  rainbowCycle(10);
  theaterChaseRainbow(50);

  ring.setBrightness(RINGMAXINTENSITY); 
}

//void fadePixels {
//for(int k = 0; k < 256; k=k+1) { r = (k/256.0)*red; g = (k/256.0)*green; b = (k/256.0)*blue; setAll(r,g,b); showring(); }

//Source: Tweaking4All.nl - Arduino - LEDring effecten voor NeoPixel en FastLED (http://www.tweaking4all.nl/hardware/arduino/adruino-led-strip-effecten/)

//}
