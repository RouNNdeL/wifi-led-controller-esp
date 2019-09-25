#include <Arduino.h>
#include <HardwareSerial.h>
#include <FS.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <ESP8266HTTPClient.h>

#include "config.h"
#include "communication.h"

WiFiServer tcp(CONFIG_TCP_PORT);

AsyncWebServer server(80);
DNSServer dns;

uint8_t *uart_buffer;
uint8_t buffer_index;
uint8_t buffer_size;

uint8_t flags;

void setup() {
    Serial.begin(9600);
    tcp.begin();

    AsyncWiFiManager wifiManager(&server, &dns);
    wifiManager.setDebugOutput(true);
    wifiManager.autoConnect(CONFIG_AP_NAME);
}

void loop() {
    uint8_t has_client = tcp.hasClient();
    if(has_client) {
        WiFiClient client = tcp.available();
        if(client) {

            /*
             * From time to time there are no bytes available, despite a client being.
             * We wait up to 10ms for some to appear, if nothing appears, the client didn't send any data
             */
            for(uint8_t i = 0; i < 10; ++i) {
                delay(1);
                if(client.available())
                    break;
            }

            while(client.available()) {
                Serial.write(client.read());
            }

            /* Wait 100ms for serial data response*/
            for(uint8_t i = 0; i < 100; ++i) {
                delay(1);
                if(Serial.available()) {
                    break;
                }
            }

            client.write(!Serial.available() ? TCP_ERROR_NO_RESPONSE : TCP_SUCCESS);

            while(Serial.available()) {
                client.write(Serial.read());
            }

            client.flush();
            client.stop();
        }
    }

    while(Serial.available()) {
        if(flags & FLAG_UART_RECEIVING) {
            if(buffer_index < buffer_size) {
                uart_buffer[buffer_index] = Serial.read();
                buffer_index++;
            } else {
                std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);

                client->setFingerprint(HTTP_SERVER_HTTPS_FINGERPRINT);
                HTTPClient https;

                if(https.begin(*client, String(HTTP_SERVER_HOST) + HTTP_SERVER_REPORT_URL)) {
                    https.POST(uart_buffer, buffer_size);
                    https.end();
                }

                flags &= ~FLAG_UART_RECEIVING;
            }
        } else {
            buffer_size = Serial.read();
            uart_buffer = (uint8_t *) malloc(buffer_size);
            buffer_index = 0;

            flags |= FLAG_UART_RECEIVING;
        }
    }
}