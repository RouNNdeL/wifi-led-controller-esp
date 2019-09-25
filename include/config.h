//
// Created by Krzysiek on 2019-08-19.
//

#ifndef ESP_CONFIG_H
#define ESP_CONFIG_H

#define VERSION_CODE 1
#define VERSION_NAME "0.0.1"
#define BUILD_DATE (String(__TIME__)+"@" + __DATE__)

#define HTTP_SERVER_HOST "iot-api.zdul.xyz"
#define HTTP_SERVER_REPORT_URL "/report_state.php"
#define HTTP_SERVER_PORT_HTTP 80
#define HTTP_SERVER_PORT_HTTPS 443
#define HTTP_SERVER_HTTPS_FINGERPRINT "01 77 78 5b ee 26 28 11 6f 66 82 4e 6f 02 87 0a c4 c1 34 42"

#ifndef CONFIG_AP_NAME
#define CONFIG_AP_NAME "LED Controller"
#endif /* CONFIG_AP_NAME */

#ifndef CONFIG_TCP_PORT
#define CONFIG_TCP_PORT 8080
#endif /* CONFIG_TCP_PORT */

#ifndef CONFIG_DEVICE_ID
#define CONFIG_DEVICE_ID "iot_generic_led"
#endif /* CONFIG_DEVICE_ID */

#endif //ESP_CONFIG_H
