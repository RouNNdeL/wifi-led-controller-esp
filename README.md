# Introduction
This is a revised version of the [ESP8266 LED Controller](https://github.com/RouNNdeL/esp8266-leds). It utilizes an ATMega328P handling the LEDs communicating over UART with an ESP8266 which handles the WiFi communication to talk to [Smart Home](https://github.com/RouNNdeL/smart-home). The device also features an external I<sup>2</sup>C EEPROM and an on board WS2812B LED. This repo contains source ESP8266 Arduino C++ files for the ESP. The AVR files can be found [here](https://github.com/RouNNdeL/wifi-led-controller-avr).

# Board
Board files are open source and are located in a separate [repo](https://github.com/RouNNdeL/wifi-led-controller).

![board](https://imgur.com/B5NNHqA.jpg)

# Schematic 
![schematic](https://github.com/RouNNdeL/wifi-led-controller/raw/master/outputs/v1/schematic.png)