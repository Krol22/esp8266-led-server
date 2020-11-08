#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>

#include <Adafruit_NeoPixel.h>
#define LED_PIN 12
#define LED_COUNT 90
#define BUFFER_LEN 1024

#ifndef STASSID
#define STSSID "Dom_72av2"
#define STAPSK "505932977"
#endif

const char* ssid = STSSID;
const char* password = STAPSK;

unsigned int localPort = 7777;
char packetBuffer[BUFFER_LEN];

WiFiUDP port;

// IP must match the IP in config.py in python folder
IPAddress ip(192, 168, 0, 150);
// Set gateway to your router's gateway
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

ESP8266WebServer server(80);

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

uint8_t N = 0;

uint8_t hue2rgb(int p, int q, int t) {
  if(t < 0) t += 1;
  if(t > 1) t -= 1;
  
  if(t < 1/6) return p + (q - p) * 6 * t;
  if(t < 1/2) return q;
  if(t < 2/3) return p + (q - p) * (2/3 - t) * 6;
  
  return p;
}

uint32_t hslToRgb(float hue, float saturation, float lightness) {
  Serial.print(hue);
  Serial.print(" ");
  Serial.print(saturation);
  Serial.print(" ");
  Serial.print(lightness);
  Serial.print(" ");
  
  uint8_t red, green, blue;

  if (saturation == 0) {
    red = green = blue = lightness;
    return strip.Color(red, green, blue);
  }

  float q = lightness < 0.5 ? lightness * (1 + saturation) : lightness + saturation - lightness * saturation;
  float p = 2 * lightness - q;
  red = hue2rgb(p, q, hue + 1/3);
  green = hue2rgb(p, q, hue);
  blue = hue2rgb(p, q, hue - 1/3);

  Serial.print(red);
  Serial.print(" ");
  Serial.print(green);
  Serial.print(" ");
  Serial.print(blue);
  Serial.println(" ");
  
  return strip.Color(red, green, blue);
}


void setup() {
  Serial.begin(115200);
  WiFi.config(ip, gateway, subnet);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  strip.begin();
  strip.show();
  strip.setBrightness(10);

  bool flash = false; 
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    strip.setPixelColor(0, strip.Color(flash ? 255 : 0, 0, 0));
    strip.show();
    flash = !flash;
  }

  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp6266")) {
    Serial.println("MDNS responder started");
  }

  server.begin();
  port.begin(localPort);

  strip.setPixelColor(0, strip.Color(0, 255, 0));
  strip.show();
}

void loop() {
  int packetSize = port.parsePacket();
  if (packetSize) {
    int len = port.read(packetBuffer, BUFFER_LEN);
    for (int i = 0; i < 90; i+=1) {
      strip.setPixelColor(i, strip.Color((uint8_t)packetBuffer[i], 0, 255));
      //Serial.print((uint8_t)packetBuffer[i]);
      //Serial.print(" - ");
      //strip.setPixelColor(i, hslToRgb((uint8_t)packetBuffer[i] / 255, 1, 1));
    }
    strip.show();
  }
  server.handleClient();
}
