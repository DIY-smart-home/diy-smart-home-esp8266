# esp8266-blinking-led
Blinking LED example for GPIO2 and GPIO5 of ESP8266

## Hardware Requirements
* ESP8266 (such as MOD-WIFI-ESP8266-DEV)
* Breadboard
* 3.3 Power supply (such as Olimex USB-PWR or YwRobot) and adapter
* 2x 5mm Red (or another color) LED
* 2x 270 Ω Resistor (red, purple, brown stripes) 1
* USB-Serial-Cable-F (USB to UART serial interface)
* 6x Jumper wires (or 8x if Olimex USB-PWR is used)

## Installation Guide
1. Connect LEDs to GPIO2 and GPIO5
2. Set GPIO0 to low
3. Connect ESP8266 to a personal computer using USB to serial cable
4. Set the power adapter to 3.3v and power on ESP8266
5. Clone the source code: `````git clone https://github.com/leon-anavi/esp8266-blinking-led`````
6. Build and flash the application on ESP8266 (sudo should be used on Linux to grant appropriate permissions)
`````
cd esp8266-blinking-led
sudo make flash
`````
