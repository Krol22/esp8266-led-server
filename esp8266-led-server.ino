#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN 12
#define LED_COUNT 96

#ifndef STASSID
#define STSSID "UPC3068195"
#define STAPSK "3Jc5jNvvmhua"
#endif

enum State {
  OFF,
  ON,
  STARTING,
  STOPPING,
  CHANGE_COLOR,
  CHANGE_BRIGHTNESS,
};

enum Animation {
  EMPTY, // placeholder for 0 value
  NONE,
  FLAME,
  GLOW,
  KIT,
};

const char* ssid = STSSID;
const char* password = STAPSK;

ESP8266WebServer server(80);

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

State state = State::OFF;
Animation prev_animation = Animation::NONE;
Animation animation = Animation::NONE;

int led_counter = 0;
int target_brightness = 0;

float glow_brightness = 0;
int glow_difference = 35;
int glow_direction = 1;

float kit_direction = 1;
float kit_counter = 11;

int brightness_sign = 0;
int brightness = 50;
bool is_on = false;

uint32_t current_color = strip.Color(255, 0, 255);

int getSign(int number) {
  if (number > 0) return 1;
  if (number < 0) return -1;
  return 0;
}

void onStrip() {
  state = State::STARTING;
  led_counter = 0;
}

void offStrip() {
  state = State::STOPPING;
  led_counter = 0;
}

void handleToggle() {
  is_on = !is_on;

  if (is_on) {
    onStrip();
  } else {
    offStrip();
  }

  server.send(200, "text/plain", "OK");
}

void handleColorChange() {
  int red = 0;
  int green = 0;
  int blue = 0;

  if (server.arg("r")) {
    red = atoi(server.arg("r").c_str());
  }  

  if (server.arg("g")) {
    green = atoi(server.arg("g").c_str());
  }

  if (server.arg("b")) {
    blue = atoi(server.arg("b").c_str());
  }

  current_color = strip.Color(red, green, blue);
  led_counter = 0;
  state = State::CHANGE_COLOR;

  server.send(200, "text/plain", "OK");
}

void handleBrightnessChange() {
  if (server.arg("brightness")) {
    target_brightness = atoi(server.arg("brightness").c_str());

    if (target_brightness > 255 || target_brightness < 1) {
      server.send(422, "text-plain", "brightness must be between 1 - 255");
      return;
    }

    if (target_brightness == brightness) {
      server.send(200, "text/plain", "OK");
      return;
    }

    brightness_sign = getSign(target_brightness - brightness);
    state = State::CHANGE_BRIGHTNESS;
    server.send(200, "text/plain", "OK");
    return;
  }

  server.send(422, "text/plain", "Missing brightness query parameter.");
}

void handleAnimation() {
  if (server.arg("animation")) {
    prev_animation = animation;
    int animation_number = atoi(server.arg("animation").c_str());

    // atoi returns 0 when "0" or letter -,-
    if (animation_number == '\0' || animation_number < 1 || animation_number > 4) {
      server.send(422, "text-aplain", "please choose number between 1 - 4");
      return;
    }

    animation = static_cast<Animation>(animation_number);

    server.send(200, "text/plain", "OK");
  }

  server.send(422, "text/plain", "Missing animation query parameter.");
}

void handleGetStatus() {
  server.send(200, "application/json", "test");
}

void animateStripe() {
  // Animation has changed
  if (animation != prev_animation) {
    for (int i = 0; i < LED_COUNT; i++) {
      strip.setPixelColor(i, current_color);
    }

    switch(animation) {
      case Animation::FLAME:
        break;
      case Animation::GLOW:
        glow_brightness = brightness;
        break;
    }

    prev_animation = animation;
    return;
  }

  switch(animation) {
    case Animation::NONE:
      break; 
    case Animation::FLAME:
      break;
    case Animation::GLOW:
      if (abs(brightness - glow_brightness) > glow_difference) {
        glow_direction = -glow_direction;
      }

      glow_brightness += (glow_direction * (0.1f));

      strip.setBrightness(glow_brightness);
      break;
    case Animation::KIT:
      if (kit_counter < 10 || kit_counter > LED_COUNT - 10) {
        kit_direction = -kit_direction;
      }

      kit_counter += (kit_direction / 3);
      
      for (int i = 0; i < LED_COUNT; i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 0));
      }

      for (int i = 1; i < 10; i++) {
        uint8_t color[4];

        *(uint32_t *)color = current_color;

        int blue = color[0]; 
        int color_change = color[0] / (10 - i);
        if (color[0] > color[0] - color_change) {
          blue = color[0] - color_change;
        }

        int green = color[1]; 
        color_change = color[1] / (10 - i);
        if (color[1] > color[1] - color_change) {
          green = color[1] - color[1] / (10 - i);
        }

        int red = color[2]; 
        color_change = color[2] / (10 - i);
        if (color[2] > color[2] - color_change) {
          red = color[2] - color[2] / (10 - i);
        }

        strip.setPixelColor(floor(kit_counter) - i, strip.Color(red, green, blue));  
        strip.setPixelColor(floor(kit_counter) + i, strip.Color(red, green, blue));  
      }

      strip.setPixelColor(floor(kit_counter), current_color);
      strip.show();

      break;
  } 
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  strip.begin();
  strip.show();
  strip.setBrightness(brightness);

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

  server.on("/toggle", handleToggle);
  server.on("/color", handleColorChange);
  server.on("/brightness", handleBrightnessChange);
  server.on("/status", handleGetStatus);
  server.on("/animation", handleAnimation);

  strip.setPixelColor(0, strip.Color(0, 255, 0));
  strip.show();
}

void loop() {
  server.handleClient();

  led_counter++;

  switch(state) {
    case State::ON: 
      animateStripe();
      break;
    case State::STARTING:
      strip.setPixelColor(led_counter, current_color);
      led_counter++;
      if (led_counter > LED_COUNT) {
        state = State::ON;
      }
      break;
    case State::STOPPING:
      strip.setPixelColor(LED_COUNT - led_counter, strip.Color(0, 0, 0));
      if (led_counter >= LED_COUNT) {
        state = State::OFF;
      }
      break;
    case State::CHANGE_COLOR:
      strip.setPixelColor(led_counter, current_color);
      if (led_counter > LED_COUNT) {
        state = State::ON;
      }
      break;
    case State::CHANGE_BRIGHTNESS:
      brightness+=brightness_sign;
      strip.setBrightness(brightness);
      if (
        brightness_sign > 0 && brightness >= target_brightness || 
        brightness_sign < 0 && brightness <= target_brightness
        ) {
        state = State::ON;
      }
    default:
      break;
  }

  strip.show();
}
