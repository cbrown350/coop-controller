setlocal

set DEV_OTA_HOST_PORT=3500
FOR /F "tokens=*" %%i in ('type .devcontainer\\.env') do @SET %%i

netsh advfirewall firewall delete rule name="udp-%DEV_OTA_HOST_PORT%-esp-ota"
netsh advfirewall firewall add rule name="udp-%DEV_OTA_HOST_PORT%-esp-ota" dir=in action=allow protocol=UDP localport=%DEV_OTA_HOST_PORT%

endlocal

