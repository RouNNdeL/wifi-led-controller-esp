#include <HardwareSerial.h>
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>
#include <ESPAsyncWiFiManager.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <FastLED.h>

#include <config.h>
#include <uart.h>

#ifdef DEBUG_ESP_CUSTOM
#define PRINTLN(a) Serial.println(a)
#define PRINT(a) Serial.print(a)
#else
#define PRINTLN(a)
#define PRINT(a)
#endif // DEBUG_ESP_CUSTOM

WiFiClient mqttTcpClient;
PubSubClient mqtt(mqttTcpClient);

AsyncWebServer server(80);
DNSServer dns;

char lightTopicPrefix[100];
char topicSetScan[100];
char sensorTopicPrefix[100];

const char* deviceNames[DEVICE_COUNT] = DEVICE_NAMES;
const char* deviceIds[DEVICE_COUNT] = DEVICE_IDS;
uint64_t temp_roms[MAX_TEMP_SENSOR_COUNT];

PROGMEM const char* discoveryLights = "{"
                                      "\"~\":\"%s\","
                                      "\"name\":\"%s\","
                                      "\"uniq_id\":\"%s\","
                                      "\"cmd_t\":\"~/%s\","
                                      "\"stat_t\":\"~/%s\","
                                      "\"schema\":\"json\","
                                      "\"brightness\": true,"
                                      "\"rgb\":true,"
                                      "\"dev\":{"
                                      "\"mf\":\"Krzysztof Zdulski\","
                                      "\"mdl\":\"LED Controller 2\","
                                      "\"sw\":\"" VERSION "\","
                                      "\"name\":\"" DEVICE_NAME "\","
                                      "\"ids\":[\"" DEVICE_ID "\", \"%08x\"],"
                                      "\"cns\":[[\"mac\", \"%s\"]]"
                                      "}"
                                      "}";

PROGMEM const char* discoverySensor = "{"
                                      "\"~\":\"%s\","
                                      "\"device_class\":\"%s\","
                                      "\"name\":\"%s\","
                                      "\"uniq_id\":\"%s\","
                                      "\"stat_t\":\"~/%s\","
                                      "\"json_attr_t\":\"~/%s\","
                                      "\"unit_of_meas\":\"%s\","
                                      "\"expire_after\": %d,"
                                      "\"force_update\": true,"
                                      "\"dev\":{"
                                      "\"mf\":\"Krzysztof Zdulski\","
                                      "\"mdl\":\"LED Controller 2\","
                                      "\"sw\":\"" VERSION "\","
                                      "\"name\":\"" DEVICE_NAME "\","
                                      "\"ids\":[\"" DEVICE_ID "\", \"%08x\"],"
                                      "\"cns\":[[\"mac\", \"%s\"]]"
                                      "}"
                                      "}";

const char* topicConfigSuffix = "config";
const char* topicSetSuffix = "set";
const char* topicStateSuffix = "state";
const char* topicAttributesSuffix = "attrs";

uint32_t last_state_update = 0;

uint8_t response_buffer[UART_BUFFER_SIZE];
uint8_t response_buffer_size = 0;
uint8_t expected_response_size;
bool uart_free;

void clear_serial() {
    while (Serial.read() >= 0);
}

void uart_send(uint8_t* data, uint16_t size) {
    for (uint16_t i = 0; i < size; ++i) {
        if (Serial.available() && Serial.peek() == UART_BUSY) {
            Serial.read();
            uart_free = false;
        }
        while (!uart_free) {
            if (Serial.available()) {
                uart_free = true;
            }
        }
        Serial.write(data[i]);
    }
}

void get_topic(char* topic, uint8_t topic_size, char* prefix, const char* device_id, const char* suffix) {
    snprintf(topic, topic_size, "%s/%s", prefix, device_id);
    strncat(topic, "/", topic_size - strlen(topic) - 1);
    strncat(topic, suffix, topic_size - strlen(topic) - 1);
}

void get_ssid(char ssid[33]) {
    struct station_config conf;
    wifi_station_get_config(&conf);
    memcpy(ssid, conf.ssid, sizeof(conf.ssid));
    ssid[32] = 0;
}

void get_ip(char ipStr[16]) {
    struct ip_info ip;
    wifi_get_ip_info(STATION_IF, &ip);
    uint32_t addr = ip.ip.addr;
    snprintf(ipStr, 16, "%u.%u.%u.%u",
             addr & 0xFF, (addr >> 8) & 0xFF,
             (addr >> 16) & 0xFF, (addr >> 24) & 0xFF);
}

LIB8STATIC_ALWAYS_INLINE void publish(const char* topic, const char* message, bool persist) {
    bool success = mqtt.publish(topic, message, persist);
    if (!success) {
        PRINT("Failed to publish to ");
        PRINTLN(topic);
    } else {
        PRINT("Published to ");
        PRINTLN(topic);
    }
}

void publish_sensor_update() {
    PRINTLN("Publish sensor update");
    sint8 rssi = WiFi.RSSI();

    if (rssi > 10) {
        return;
    }

    char ssid[33];
    char ip[16];
    char topic[120];
    char message[64];

    get_topic(topic, sizeof(topic), sensorTopicPrefix, SIGNAL_SENSOR_ID, topicStateSuffix);
    snprintf(message, sizeof(message), "%d", rssi);
    publish(topic, message, true);

    get_ssid(ssid);
    get_ip(ip);
    snprintf(message, sizeof(message), R"({"rssi":%d,"ssid":"%s","ip":"%s"})", rssi, ssid, ip);

    get_topic(topic, sizeof(topic), sensorTopicPrefix, SIGNAL_SENSOR_ID, topicAttributesSuffix);
    publish(topic, message, true);
}

void request_temperature() {
    uint8_t data[] = {UART_BEGIN, CMD_GET_TEMPS, 0, UART_END};
    uart_send(data, sizeof(data));
}

void publish_temperature(const uint8_t* temperatures, uint8_t count) {
    char topic[120];
    char message[10];
    for (uint8_t i = 0; i < count; ++i) {
        int16_t temperature = temperatures[i * 2 + 1] << 8 | temperatures[i * 2];

        // Check if the data is valid
        if (temperature == INT16_MIN) {
            continue;
        }

        char tempId[sizeof(TEMP_SENSOR_ID_PREFIX) + 9];

        snprintf(tempId, sizeof(tempId), "%s_%" PRIx64, TEMP_SENSOR_ID_PREFIX, temp_roms[i]);
        get_topic(topic, sizeof(topic), sensorTopicPrefix, tempId, topicStateSuffix);
        snprintf(message, sizeof(message), "%.1f", (float) temperature * 0.0625);
        publish(topic, message, true);
    }
}

void handle_data() {
    uint8_t* data = response_buffer + 3;
    switch (response_buffer[1]) {
        case CMD_DEVICE_RESPONSE: {
            uint8_t device_index = data[0];
            uint8_t brightness = data[1];
            uint8_t flags = data[2];
            CRGB color(data[3], data[4], data[5]);
            uint8_t effect = data[6];

            StaticJsonDocument<JSON_BUFFER_SIZE> json;
            json["state"] = flags & (1 << 0) ? "ON" : "OFF";
            json["brightness"] = brightness;
            json["color"]["r"] = color.r;
            json["color"]["g"] = color.g;
            json["color"]["b"] = color.b;

            char topic[120];
            get_topic(topic, sizeof(topic), lightTopicPrefix, deviceIds[device_index], topicStateSuffix);

            char message[JSON_BUFFER_SIZE];
            serializeJson(json, message);
            publish(topic, message, true);
            break;
        }
        case CMD_GET_TEMPS_RESPONSE: {
            uint8_t temp_count = data[0];
            publish_temperature(data + 1, temp_count);
            break;
        }
    }
}

void handle_uart() {
    while (Serial.available()) {
        uint8_t byte = Serial.read();
        if (response_buffer_size == 0) {
            if (byte == UART_BUSY) {
                uart_free = false;
            } else if (byte == UART_FREE) {
                uart_free = true;
            }

            if (byte != UART_BEGIN) {
                clear_serial();
                return;
            }
        }

        response_buffer[response_buffer_size++] = byte;
        if (response_buffer_size == 3) {
            expected_response_size = byte;
        }

        if (response_buffer_size == expected_response_size + 4) {
            if (byte == UART_END) {
                response_buffer_size = 0;
                handle_data();
            } else {
                clear_serial();
                response_buffer_size = 0;
            }
        }
    }
}

void request_all_devices() {
    uint8_t data[] = {UART_BEGIN, CMD_GET_ALL_DEVICES, 0, UART_END};
    uart_send(data, sizeof(data));
}

void sendColor(uint8_t device_index, CRGB color) {
    uint8_t data[] = {UART_BEGIN, CMD_SET_COLOR, 4, device_index, color.r, color.g, color.b, UART_END};
    uart_send(data, sizeof(data));
}

void sendState(uint8_t device_index, bool state) {
    uint8_t data[] = {UART_BEGIN, CMD_SET_STATE, 2, device_index, state, UART_END};
    uart_send(data, sizeof(data));
}

void sendBrightness(uint8_t device_index, uint8_t brightness) {
    uint8_t data[] = {UART_BEGIN, CMD_SET_BRIGHTNESS, 2, device_index, brightness, UART_END};
    uart_send(data, sizeof(data));
}

void mqttCallback(char* topic, uint8_t* payload, uint16_t length) {
    PRINT("Received payload for topic: ");
    PRINTLN(topic);
    char deviceId[32];
    sscanf(topic, topicSetScan, deviceId);
    PRINT("Device id: ");
    PRINTLN(deviceId);
    for (uint8_t i = 0; i < DEVICE_COUNT; ++i) {
        if (!strcmp(deviceId, deviceIds[i])) {
            StaticJsonDocument<JSON_BUFFER_SIZE> json;
            deserializeJson(json, payload, DeserializationOption::NestingLimit(3));

            if (json.containsKey("color") && json["color"].is<JsonObject>()) {
                JsonObject color = json["color"].as<JsonObject>();
                if (color.containsKey("r") && color.containsKey("g") && color.containsKey("b")
                    && color["r"].is<uint8_t>() && color["g"].is<uint8_t>() && color["b"].is<uint8_t>()) {
                    CRGB c = CRGB(color["r"], color["g"], color["b"]);
                    sendColor(i, c);
                }
            }

            if (json.containsKey("brightness") && json["brightness"].is<uint8_t>()) {
                sendBrightness(i, json["brightness"]);
            }

            if (json.containsKey("state") && json["state"].is<char*>()) {
                const char* state = json["state"];
                if (!strcmp(state, "ON")) {
                    sendState(i, true);
                } else if (!strcmp(state, "OFF")) {
                    sendState(i, false);
                }
            }
        }
    }
}

void reboot() {
    delay(RESTART_DELAY * 1000);
    ESP.restart();
}

uint8_t request_temp_info(uint64_t* roms, uint8_t max_roms) {
    clear_serial();

    uint8_t data[] = {UART_BEGIN, CMD_GET_TEMPS_INFO, 0, UART_END};
    uart_send(data, sizeof(data));

    uint8_t buf[64 + MAX_TEMP_SENSOR_COUNT * 8];
    uint8_t success = Serial.readBytesUntil(UART_END, buf, sizeof(buf));
    if (!success) {
        reboot();
    }

    uint8_t start = 0;
    while (buf[start] != UART_BEGIN) {
        start++;
        if (start >= sizeof(buf)) {
            reboot();
        }
    }

    if (buf[start + 1] != CMD_GET_TEMPS_COUNT_RESPONSE) {
        reboot();
    }

    uint8_t temp_count = buf[start + 3];
    if (temp_count > max_roms) {
        temp_count = max_roms;
    }

    for (uint8_t i = 0; i < temp_count; ++i) {
        memcpy(roms + i * sizeof(uint64_t), buf + 4 + i * sizeof(uint64_t), sizeof(uint64_t));
    }

    return temp_count;
}

void setup() {
    Serial.begin(BAUD);

    AsyncWiFiManager wifiManager(&server, &dns);
#ifdef DEBUG_ESP_CUSTOM
    wifiManager.setDebugOutput(true);
#endif // DEBUG_ESP_CUSTOM
    wifiManager.autoConnect(DEVICE_NAME);

    //mqttTcpClient.setInsecure();
    mqtt.setServer(MQTT_BROKER_HOST, MQTT_BROKER_PORT);
    mqtt.setCallback(mqttCallback);
    mqtt.setBufferSize(512); // Temporally increase buffer size for discovery payloads

    char nodeId[sizeof(MQTT_NODE_ID_PREFIX) + 16];
    snprintf(nodeId, sizeof(nodeId), "%s_%08x", MQTT_NODE_ID_PREFIX, ESP.getChipId());
    mqtt.connect(nodeId, MQTT_USER, MQTT_PASSWORD);

    size_t jsonSize = max(strlen(discoveryLights), strlen(discoverySensor)) + 256;
    mqtt.setBufferSize(jsonSize + 64);

    snprintf(lightTopicPrefix, sizeof(lightTopicPrefix), "%s/light/%s", MQTT_DISCOVERY_PREFIX, nodeId);
    snprintf(sensorTopicPrefix, sizeof(sensorTopicPrefix), "%s/sensor/%s", MQTT_DISCOVERY_PREFIX, nodeId);
    snprintf(topicSetScan, sizeof(topicSetScan), "%s/%%[^/]/%s", lightTopicPrefix, topicSetSuffix);

    char topic[120];
    char deviceUid[sizeof(nodeId) + 32];
    char* discoveryJson = static_cast<char*>(malloc(jsonSize));

    char macStr[18] = {0};
    uint8_t mac[6];
    WiFi.macAddress(mac);
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    PRINTLN("Starting discovery");
    for (uint8_t i = 0; i < DEVICE_COUNT; ++i) {
        snprintf(topic, sizeof(topic), "%s/%s", lightTopicPrefix, deviceIds[i]);
        snprintf(deviceUid, sizeof(deviceUid), "%s_%s", nodeId, deviceIds[i]);
        snprintf(discoveryJson, jsonSize, discoveryLights, topic, deviceNames[i],
                 deviceUid, topicSetSuffix, topicStateSuffix, ESP.getChipId(), macStr);


        bool success;
        get_topic(topic, sizeof(topic), lightTopicPrefix, deviceIds[i], topicSetSuffix);
        success = mqtt.subscribe(topic);
        if (!success) {
            PRINT("Failed to subscribe to topic for device ");
            PRINTLN(deviceNames[i]);
        } else {
            PRINT("subscribed to topic ");
            PRINTLN(topic);
        }

        get_topic(topic, sizeof(topic), lightTopicPrefix, deviceIds[i], topicConfigSuffix);
        publish(topic, discoveryJson, true);
    }

    // Signal sensor discovery
    snprintf(deviceUid, sizeof(deviceUid), "%s_%s", nodeId, SIGNAL_SENSOR_ID);
    snprintf(topic, sizeof(topic), "%s/%s", sensorTopicPrefix, SIGNAL_SENSOR_ID);

    snprintf(discoveryJson, jsonSize, discoverySensor, topic, "signal_strength", SIGNAL_SENSOR_NAME, deviceUid,
             topicStateSuffix, topicAttributesSuffix, "dBm", PUBLISH_PERIOD * 2, ESP.getChipId(), macStr);


    get_topic(topic, sizeof(topic), sensorTopicPrefix, SIGNAL_SENSOR_ID, topicConfigSuffix);
    publish(topic, discoveryJson, true);

    uint8_t temp_count = request_temp_info(temp_roms, sizeof(temp_roms) / sizeof(temp_roms[0]));

    for (uint8_t i = 0; i < temp_count; ++i) {
        char tempId[sizeof(TEMP_SENSOR_ID_PREFIX) + 9];
        char tempName[sizeof(TEMP_SENSOR_NAME_PREFIX) + 6];

        snprintf(tempId, sizeof(tempId), "%s_%" PRIx64, TEMP_SENSOR_ID_PREFIX, temp_roms[i]);
        snprintf(tempName, sizeof(tempName), "%s%u", TEMP_SENSOR_NAME_PREFIX, i + 1);

        snprintf(deviceUid, sizeof(deviceUid), "%s_%s", nodeId, tempId);
        snprintf(topic, sizeof(topic), "%s/%s", sensorTopicPrefix, tempId);

        snprintf(discoveryJson, jsonSize, discoverySensor, topic, "temperature", tempName, deviceUid,
                 topicStateSuffix, topicAttributesSuffix, "°C", PUBLISH_PERIOD * 2, ESP.getChipId(), macStr);


        get_topic(topic, sizeof(topic), sensorTopicPrefix, tempId, topicConfigSuffix);
        publish(topic, discoveryJson, true);
    }

    free(discoveryJson);
    mqtt.setBufferSize(256);
#ifndef DEBUG_ESP_CUSTOM
    request_all_devices();
#endif // DEBUG_ESP_CUSTOM
    publish_sensor_update();
    request_temperature();
}

void loop() {
    if (millis() > last_state_update + PUBLISH_PERIOD * 1000) {
        last_state_update = millis();
        publish_sensor_update();
        request_temperature();
    }

    if (!mqtt.loop()) {
        reboot();
    }

    handle_uart();
}