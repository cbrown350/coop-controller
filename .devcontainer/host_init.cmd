REM Create .devcontainer\\.env if it doesn't exist
if not exist .devcontainer\\.env copy NUL .devcontainer\\.env

REM Bring USB devices into wsl on Windows - add USB vendor:product IDs here and in container_init.sh
REM The first time these run it will need admin privileges to bind 
REM esp32 uart
start cmd /C c:\\Progra~1\\usbipd-win\\usbipd.exe wsl attach -ai 10c4:ea60 || ver>nul
REM esp-prog
start cmd /C c:\\Progra~1\\usbipd-win\\usbipd.exe wsl attach -ai 0403:6010 || ver>nul

REM Start and stop usbipd.exe in the background with automatic attach
REM start /B c:\\Progra~1\\usbipd-win\\usbipd.exe wsl attach -ai 10c4:ea60
REM start /B c:\\Progra~1\\usbipd-win\\usbipd.exe wsl attach -ai 0403:6010
REM wmic process where "name like '%%usbipd%%' and commandline like '%%wsl%%'" delete


REM Set up udev rules for host WSL
wsl -u root if [[ ! -e /etc/udev/rules.d/60-openocd.rules ]]; then wget -O /etc/udev/rules.d/60-openocd.rules https://raw.githubusercontent.com/raspberrypi/openocd/rp2040/contrib/60-openocd.rules; /lib/systemd/systemd-udevd --daemon; udevadm control --reload-rules; fi

wsl -u root if [[ ! -e /etc/udev/rules.d/99-platformio-udev.rules ]]; then wget -O /etc/udev/rules.d/99-platformio-udev.rules https://raw.githubusercontent.com/platformio/platformio-core/develop/platformio/assets/system/99-platformio-udev.rules; /lib/systemd/systemd-udevd --daemon; udevadm control --reload-rules; fi


REM Set up Windows Firewall
setlocal
SET DEV_OTA_HOST_PORT=3500
FOR /F "tokens=*" %%i in ('type .devcontainer\\.env') do @SET %%i
netsh advfirewall firewall show rule name="udp-%DEV_OTA_HOST_PORT%-esp-ota" || powershell "start .devcontainer\\win_firewall_setup.cmd -v runAs"
endlocal