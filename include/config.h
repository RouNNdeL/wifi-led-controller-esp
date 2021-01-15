//
// Created by Krzysiek on 2019-08-19.
//

#ifndef ESP_CONFIG_H
#define ESP_CONFIG_H

#define UART_BUFFER_SIZE 200

#ifndef CONFIG_AP_NAME
#define CONFIG_AP_NAME "Bed LEDs"
#endif /* CONFIG_AP_NAME */

// Keep these names as short as possible. No longer than 20-30 chars
#define DEVICE_COUNT 3
#define DEVICE_IDS {"bed_onboard", "bed_top","bed_bottom"}
#define DEVICE_NAMES {"Onboard Led - Bed", "Top Bed Light", "Bottom Bed Light"}

#define JSON_BUFFER_SIZE 256

#define MQTT_BROKER_HOST "192.168.1.160"
#define MQTT_BROKER_PORT 1883
#define MQTT_USER "esp"
#define MQTT_PASSWORD "password"
#define MQTT_NODE_ID_PREFIX "esp_led"
#define MQTT_DISCOVERY_PREFIX "homeassistant"

#endif //ESP_CONFIG_H
