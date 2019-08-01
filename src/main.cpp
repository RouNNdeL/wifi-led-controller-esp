#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <HardwareSerial.h>
#include <FS.h>

ESP8266WebServer httpd;


ICACHE_RAM_ATTR void move_right() {
    Serial.write('r');
    httpd.send(200);
}

ICACHE_RAM_ATTR void move_left() {
    Serial.write('l');
    httpd.send(200);
}

void setup() {
    Serial.begin(9600);

    WiFi.mode(WIFI_STA);

    WiFi.begin("********", "********");
    while(!WiFi.isConnected()) {delay(1);};
    Serial.println(WiFi.localIP());

    httpd.on("/r", HTTP_POST, move_right);
    httpd.on("/l", HTTP_POST, move_left);
    httpd.on("/", []() {
        File file = SPIFFS.open("/index.html", "r");
        httpd.streamFile(file, "text/html");
        file.close();
    });

    httpd.begin();

    SPIFFS.begin();
}

void loop() {
    httpd.handleClient();
}