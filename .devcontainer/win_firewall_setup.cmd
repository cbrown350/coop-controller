setlocal
set OTA_PORT=3500
FOR /F "tokens=*" %%i in ('type .devcontainer\\.env') do @SET %%i

netsh advfirewall firewall delete rule name="udp-%OTA_PORT%-esp-ota"
netsh advfirewall firewall add rule name="udp-%OTA_PORT%-esp-ota" dir=in action=allow protocol=UDP localport=%OTA_PORT%

endlocal
