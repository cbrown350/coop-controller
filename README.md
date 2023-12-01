# Chicken Coop Controller

Based on the ESP32 controller (ESP32-DevkitC)

## Features

* [x] OTA Updater
    * [x] Via PlatformIO directly
    * [x] Via HTTPS file server
* [x] WiFi Config
* [ ] Zero Crossing Function (Pin ISR)
* [ ] Floodlight Dimmer (for run and nesting box)
    * [ ] Leading Edge Phase Dimming
* [ ] Sunrise/Sunset Trigger
    * [ ] Time Zones
    * [ ] Daylight Savings
* [ ] OLED Display Controller
* [ ] Roost Door Controller
    * [ ] Door Open Sense
    * [ ] Door Close Sense
    * [ ] Motor Close Power Sense (ADC)
    * [ ] Motor Direction (2 Lines)
    * [ ] Trigger When Open/Close Sensed (Pins ISR)
* [ ] Watchdog/Heartbeat Indicator
* [ ] Temperature ADC Base
* [ ] Roost Temperature (ADC)
* [ ] Outside Temperature (ADC)
* [ ] Water Temperature (ADC)
* [ ] Water Heater Controller
    * [ ] Pulse Skip Modulation Power Control? Only need On/Off
* [ ] MQTT
    * [ ] config via WiFi setup page
* [ ] Web Page Dashboard
    * [ ] Status
    * [ ] Basic Controls
    * [ ] Settings Changes (WiFi, MQTT, etc.)
* [ ] Box Open Light Sensor (ADC)
* [ ] Low Water Level Sensor
* [ ] Food Level Sensor (ADC)
* [ ] Single LED Controller
    * [ ] Red LED
    * [ ] Green LED (DAC)
* [x] Factory Reset Function (Boot Switch Duration 10s)
* [ ] WiFi Reset Function (Boot Switch Duration 30s, Red LED Slow Flash)
* [ ] Buzzer Controller (DAC)
* [ ] Water Flow Meter

Future Functions

* [ ] Error Email
* [ ] Andon Light on Box

Future ADC Functions (not enough channels now)

* [ ] Outdoor Brightness Sensor (ADC)
* [ ] Heater Current Sensor (ADC)
* [ ] Spotlight Sensor (ADC)

## Quick Start

1. If using devcontainers in VS Code, create a `.env` file under the `.devcontainers` folder and put your GitHub token in under `GH_TOKEN` and the `DEV_OTA_REMOTE_DEVICE_IP` as the IP address on your network of the ESP32 device you're working with (for direct OTA updates, you need to set `DEV_OTA_REMOTE_DEVICE_IP` in the `.env` file in the project root or set the upload port in the ini file)
2. Create a `.env` file in the project root to hold private values (see `include/coop_settings.h` for a list or example.env)
3. Install recommended VS Code extensions you may need (platformio, etc.)
4. On Windows if not using devcontainers and using the esp-proj JTAG tool, install Zadig ([Zadig - USB driver installation made easy (akeo.ie)](https://zadig.akeo.ie/)); you may need other USB drivers; adjust any ports for debug or monitoring in the platformio.ini file
5. If using devcontainers on Windows, you must:
    1. Install a Linux distro for WSL
    2. Install Docker Desktop
    3. Install USBIPD (https://github.com/dorssel/usbipd-win/releases) in Windows
    4. If using Ubuntu (you'll have to find comparable usbip install for other distros):
        1. sudo apt install linux-tools-5.4.0-77-generic hwdata
        2. sudo update-alternatives --install /usr/local/bin/usbip usbip /usr/lib/linux-tools/5.4.0-77-generic/usbip 20
    5. Setup any USB Vendor/Product IDs for devices you're using in the container in `.devcontainer/container_init.sh` and `.devcontainer/host_init.cmd` (esp-prog and the ESP32 IDs are already set there)
6. See the schematic and other information under the `docs` folder
7. To create an OTA update package (json manifest and firmware binaries), build the project and build the file system and it will appear in `.pio/build/ota`