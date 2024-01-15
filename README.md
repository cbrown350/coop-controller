# Chicken Coop Controller

Based on the ESP32 controller (ESP32-DevkitC)

## Features

* [x] OTA Updater
    * [x] Via PlatformIO directly
    * [x] Via HTTPS file server
    * [ ] Via web page
* [x] WiFi Config
    * [ ] Reconfig via web page
* [x] Set time from NTP server
    * [x] Get timezone and DST from WiFi setup
    * [ ] Get timezone and DST from web page
    * [ ] Infer timezone from?
* [ ] Sunrise/Sunset Trigger
    * [ ] Time Zones
    * [ ] Daylight Savings
* [ ] Zero Crossing Function for Dimming (Pin ISR)
* [ ] Floodlight Dimmers (for run and nesting box)
    * [ ] Leading Edge Phase Dimming
    * [ ] Get settings from WiFi setup
    * [ ] Get settings from web page
    * [ ] Get settings from Home Assistant MQTT
* [ ] OLED Display Controller
    * [ ] Templates different display data
    * [ ] Get settings from web page
* [ ] Roost Door Controller
    * [ ] Door Open Sense
    * [ ] Door Close Sense
    * [ ] Motor Close Power Sense (ADC)
    * [ ] Motor Direction (2 Lines)
    * [ ] Trigger When Open/Close Sensed (Pins ISR)
    * [ ] MQTT status and control and settings?
    * [ ] Web page status and settings
* [ ] Watchdog/Heartbeat Indicator
    * [ ] MQTT status
    * [ ] Web page status
* [ ] Temperature ADC Base Object
    * [ ] MQTT status
    * [ ] Web page status
* [ ] Roost Temperature (ADC)
* [ ] Outside Temperature (ADC)
* [ ] Water Temperature (ADC)
* [ ] Water Heater Controller
    * [ ] Pulse Skip Modulation Power Control? Only need On/Off
    * [ ] MQTT status and control
    * [ ] Web page status
* [ ] MQTT
    * [x] config via WiFi setup page
    * [ ] Config via web page
    * [ ] Report data to Home Assistant
    * [ ] Home Assistant auto-discovery
* [ ] Web Page Dashboard
    * [ ] Status
    * [ ] Basic Controls
    * [ ] Settings Changes (WiFi, MQTT, Syslog, etc.)
* [ ] Box Open Light Sensor (ADC)
    * [ ] MQTT status
    * [ ] Web page status
* [ ] Low Water Level Sensor
    * [ ] MQTT status
    * [ ] Web page status
* [ ] Food Level Sensor (ADC)
    * [ ] MQTT status
    * [ ] Web page status
* [ ] Single LED Controller
    * [ ] Red LED, watchdog?
    * [ ] Green LED (DAC)?
* [x] Factory Reset Function (Boot Switch Duration 10s)
* [ ] WiFi Reset Function (Boot Switch Duration 30s, Red LED Slow Flash)
* [ ] Buzzer Controller (DAC)
* [ ] Water Flow Meter
    * [ ] MQTT status
    * [ ] Web page status
* [x] Syslog support
    * [ ] Buffer messages when not connected
    * [ ] Add server settings to WiFi config setup
    * [ ] MQTT status
    * [ ] Web page status and settings
* [ ] Backup/Restore config settings

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
    3. Install USBIPD (https://github.com/dorssel/usbipd-win/releases) in Windows (instructions may change and updates can be viewed at: https://learn.microsoft.com/en-us/windows/wsl/connect-usb#attach-a-usb-device)
    4. Find your Linux kernel version (uname -r) -> kernel\_version
    5. If using Ubuntu (you'll have to find comparable usbip install for other distros):
        1. sudo apt install libusb-dev linux-tools-generic hwdata (may have to find a specific version closest to your kernel version, e.g. linux-tools-${kernel\_version}-generic)
        2. sudo update-alternatives --install /usr/local/bin/usbip usbip /usr/lib/linux-tools/${kernel\_version}-generic/usbip 20 (may have to use specific version using your kernel or installed version, e.g. linux-tools/${kernel\_version}-generic)
    6. Setup any USB Vendor/Product IDs for devices you're using in the container in `.devcontainer/container_init.sh` and `.devcontainer/host_init.cmd` (esp-prog and the ESP32 IDs are already set there)
    7. You may need to restart the container after the first time you run it to get the USB devices to show up
    8. USB ports may need to be specified in the platformio.ini file for upload\_port, monitor\_port and JTAG tool
6. See the schematic and other information under the `docs` folder
7. To create an OTA update package (json manifest and firmware binaries), build the project and build the file system and it will appear in `.pio/build/ota`
8. JTAG (esp-prog) often requires holding down the boot button on the ESP32 board while uploading or debugging
9. To debug a test you must build it before starting a debug session (e.g. `pio test -e esp32-debug --without-uploading --without-testing`) and use the skip Pre-Debug option to avoid building again
10. If you just want to use a serial monitor directly on the command line, tio is installed in the devcontainer and can be used with `tio -b 115200 /dev/ttyUSB0` (or whatever port you're using)