//#define FASTLED_ESP8266_RAW_PIN_ORDER
//#define FASTLED_INTERRUPT_RETRY_COUNT 0
#include <FastLED.h>

#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <ESP8266mDNS.h>
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#include <WiFiUdp.h>

enum UDP_COMMAND {
    COMMAND_PING = 0,
    COMMAND_GETMD = 1,
    COMMAND_RENDER = 2,
    COMMAND_RAINBOW = 3,
};

struct metadata_t {
    uint8_t version;
    uint8_t leds_length;
    uint8_t leds_type;
};

const char WIFI_PASSWORD[]    = "lightlayer";        // Password
const char WIFI_SSID_PREFIX[] = "LightLayer";  // Local AP SSID Prefix

const short UDP_PORT  =  1337;

#define NUM_LEDS  150
#define FRAMERATE 120
#define DATA_PIN  D7
#define CLOCK_PIN D5

CRGB leds[NUM_LEDS];
bool rainbow = true;

metadata_t md = {
  .version = 1,
  .leds_length = NUM_LEDS,
  .leds_type = APA102,
};

uint8_t pong[] = {13,37};


WiFiManager wifiManager;
WiFiUDP Udp;

void setupWiFi()
{
  String ap_name = WIFI_SSID_PREFIX + String(ESP.getChipId());
  wifiManager.autoConnect(ap_name.c_str(), WIFI_PASSWORD);
  
  if (!MDNS.begin(ap_name.c_str())) {
    // Error
  }

  MDNS.addService("lightlayer", "udp", UDP_PORT);
}

void sendUDP(uint8_t * buffer, uint8_t len) {
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(buffer, len);
    Udp.endPacket();
}

void setup() {
  // LED stuff
  pinMode(LEDS_EN_PIN, OUTPUT);
  FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<WS2812, DATA_PIN, RGB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // WiFi & network stuff
  setupWiFi();

  Udp.begin(UDP_PORT);
}

void loop() {
    uint8_t command;
    int packet_size = Udp.parsePacket();
    if (packet_size) {
        Udp.read(&command, sizeof(command));
        switch (command) {
            case COMMAND_PING:
                sendUDP((uint8_t*)&pong, sizeof(pong));
                break;
                
            case COMMAND_GETMD:
                sendUDP((uint8_t*)&md, sizeof(md));
                break;
            
            case COMMAND_RENDER:
                rainbow = false;
                memset((uint8_t*)&leds, 0, sizeof(leds));
                Udp.read((uint8_t*)&leds, MIN(packet_size-1, sizeof(leds)));
                break;
                
            case COMMAND_RAINBOW:
                rainbow = true;
                break;
                     
        }
    }

    if (rainbow) {
      static uint8_t hue = 0;
      fill_rainbow(leds, NUM_LEDS, ++hue, 7);
    }
    
    FastLED.delay(1000/FRAMERATE);
}
