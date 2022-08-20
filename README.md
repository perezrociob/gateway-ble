# gateway-ble

BLE (Bluetooth Low Energy) to WiFi gateway on ESP32 used in my final degree project in Electronics Engineering. The project consisted on the development of a Wireless Sensor Network (WSN) using BLE technology.

## About the Wireless Sensor Network
- Local nodes: Capacitive Soil Moisture Sensor and HM-10 BLE Modules  [HM-10 Datasheet](https://people.ece.cornell.edu/land/courses/ece4760/PIC32/uart/HM10/DSD%20TECH%20HM-10%20datasheet.pdf).
- Gateway: Nodemcu Esp32 Wifi + Bluetooth 4.2 Iot Wroom ESP32

## Local nodes basic configuration
Configured via AT commands
- Peripheral
- Adquisition Mode

## Limitations
-Some HM-10 modules are clones, and do not allow the required AT commands in configuration. To flash the original firmware read the following [tutorial](https://circuitdigest.com/microcontroller-projects/how-to-flash-the-firmware-on-cloned-hm-10-ble-module-using-arduino-uno). **It only works on clone MLT-BT05.**
- HM-10 modules do not support a mesh network. The built network is point-to-point and connects to one server (HM-10 module) at a time.
