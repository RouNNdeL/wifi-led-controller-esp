#include <HardwareSerial.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <WebSocketsServer.h>

#include <config.h>
#include <uart.h>

WebSocketsServer webSocket(81);

AsyncWebServer server(80);
DNSServer dns;

uint8_t response_buffer[BUFFER_SIZE];
uint8_t response_buffer_size = 0;
uint8_t expected_response_size;

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED: {
            uint8_t resp[] = {(uint8_t) webSocket.connectedClients()};
            webSocket.sendBIN(num, resp, 1);
            break;
        }
        case WStype_BIN:
            Serial.write(payload, length);
            break;
    }
}

void clear_serial() {
    while (Serial.available()) {
        Serial.read();
    }
}

void socket_send_data(uint8_t *data, size_t size) {
    webSocket.broadcastBIN(data, size);
}

void handle_uart() {
    while (Serial.available()) {
        uint8_t byte = Serial.read();
        if (response_buffer_size == 0) {
            if (byte != UART_BEGIN) {
                clear_serial();
                uint8_t data[] = {byte};
                socket_send_data(data, sizeof(data));
                return;
            }
        }

        response_buffer[response_buffer_size++] = byte;
        if(response_buffer_size == 6) {
            expected_response_size = byte;
        }

        if(response_buffer_size == expected_response_size + 7) {
            if(byte == UART_END) {
                socket_send_data(response_buffer, response_buffer_size);
                response_buffer_size = 0;
            } else {
                clear_serial();
                uint8_t data[] = {UART_INVALID_SEQUENCE};
                socket_send_data(data, sizeof(data));
                response_buffer_size = 0;
            }
        }
    }
}

void setup() {
    Serial.begin(BAUD);

    AsyncWiFiManager wifiManager(&server, &dns);
    wifiManager.setDebugOutput(true);
    wifiManager.autoConnect(CONFIG_AP_NAME);

    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
}

void loop() {
    webSocket.loop();

    handle_uart();
}