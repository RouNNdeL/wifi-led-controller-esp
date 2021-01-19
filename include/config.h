//
// Created by Krzysiek on 2019-08-19.
//

#ifndef ESP_CONFIG_H
#define ESP_CONFIG_H

#define UART_BUFFER_SIZE 200
#define VERSION "2.0.2"

#ifndef DEVICE_NAME
#define DEVICE_NAME "LED Controller - Test"
#endif /* DEVICE_NAME */

#ifndef DEVICE_ID
#define DEVICE_ID "led_test"
#endif /* DEVICE_ID */

// Keep these names as short as possible. No longer than 20-30 chars
#ifndef DEVICE_COUNT
#define DEVICE_COUNT 4
#endif /* DEVICE_COUNT */

#ifndef DEVICE_IDS
#define DEVICE_IDS {"test_onboard", "test_1", "test_2", "test_3"}
#endif /* DEVICE_IDS */

#ifndef DEVICE_NAMES
#define DEVICE_NAMES {"Onboard Led - Test", "Test 1", "Test 2", "Test 3"}
#endif /* DEVICE_NAMES */

#ifndef SIGNAL_SENSOR_NAME
#define SIGNAL_SENSOR_NAME "Test - Signal"
#endif /* SIGNAL_SENSOR_NAME */

// Has to be different from the other IDs
#ifndef SIGNAL_SENSOR_ID
#define SIGNAL_SENSOR_ID "test_signal"
#endif /* SIGNAL_SENSOR_ID */
// In Seconds
#ifndef SIGNAL_PUBLISH_PERIOD
#define SIGNAL_PUBLISH_PERIOD 60
#endif /* SIGNAL_PUBLISH_PERIOD */

#ifndef JSON_BUFFER_SIZE
#define JSON_BUFFER_SIZE 256
#endif /* JSON_BUFFER_SIZE */
// In Seconds
#ifndef RESTART_DELAY
#define RESTART_DELAY 30
#endif /* RESTART_DELAY */

#ifndef MQTT_BROKER_HOST
#define MQTT_BROKER_HOST "test.mosquitto.org"
#endif /* MQTT_BROKER_HOST */

#ifndef MQTT_BROKER_PORT
#define MQTT_BROKER_PORT 1883
#endif /* MQTT_BROKER_PORT */

#ifndef MQTT_USER
#define MQTT_USER "esp"
#endif /* MQTT_USER */

#ifndef MQTT_PASSWORD
#define MQTT_PASSWORD "password"
#endif /* MQTT_PASSWORD */

#ifndef MQTT_NODE_ID_PREFIX
#define MQTT_NODE_ID_PREFIX "esp_led"
#endif /* MQTT_NODE_ID_PREFIX */

#ifndef MQTT_DISCOVERY_PREFIX
#define MQTT_DISCOVERY_PREFIX "homeassistant"
#endif /* MQTT_DISCOVERY_PREFIX */

#endif //ESP_CONFIG_H
