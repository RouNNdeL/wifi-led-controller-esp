#include <Arduino.h>
#include <HardwareSerial.h>
#include <FS.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <ESP8266HTTPClient.h>
#include <WebSocketsServer.h>

#include "config.h"
#include "communication.h"

WebSocketsServer webSocket(81);

AsyncWebServer server(80);
DNSServer dns;

uint8_t *uart_buffer;
uint8_t buffer_index;
uint8_t buffer_size;

uint8_t flags;

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {

    switch (type) {
        case WStype_CONNECTED: {
            uint8_t resp[] = {0xA0};
            webSocket.sendBIN(num, resp, 1);
            break;
        }
        case WStype_BIN:
            Serial.write(payload, length);
            break;
    }
}

void setup() {
    Serial.begin(9600);

    AsyncWiFiManager wifiManager(&server, &dns);
    wifiManager.setDebugOutput(true);
    wifiManager.autoConnect(CONFIG_AP_NAME);

    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
}

void loop() {
    webSocket.loop();

    size_t size = (size_t) Serial.available();
    if (size > 0) {
        uint8_t response_buffer[size];
        Serial.read((char *) response_buffer, size);
        webSocket.broadcastBIN(response_buffer, size);
    }
}