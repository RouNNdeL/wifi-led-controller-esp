//
// Created by Krzysiek on 2019-08-19.
//

#ifndef ESP_CONFIG_H
#define ESP_CONFIG_H

#define UART_BUFFER_SIZE 200
#define VERSION "2.0.1"

#ifndef DEVICE_NAME
#define DEVICE_NAME "Bed LEDs"
#endif /* DEVICE_NAME */

#ifndef DEVICE_ID
#define DEVICE_ID "led_bed"
#endif /* DEVICE_ID */

// Keep these names as short as possible. No longer than 20-30 chars
#define DEVICE_COUNT 3
#define DEVICE_IDS {"bed_onboard", "bed_bottom", "bed_top"}
#define DEVICE_NAMES {"Onboard Led - Bed", "Bottom Bed Light", "Top Bed Light"}

#define SIGNAL_SENSOR_NAME "Table - Signal"
// Has to be different from the other IDs
#define SIGNAL_SENSOR_ID "table_signal"
// In Seconds
#define SIGNAL_PUBLISH_PERIOD 60

#define JSON_BUFFER_SIZE 256
// In Seconds
#define RESTART_DELAY 30

#define MQTT_BROKER_HOST "192.168.1.10"
#define MQTT_BROKER_PORT 1883
#define MQTT_USER "esp"
#define MQTT_PASSWORD "password"
#define MQTT_NODE_ID_PREFIX "esp_led"
#define MQTT_DISCOVERY_PREFIX "homeassistant"

#endif //ESP_CONFIG_H
