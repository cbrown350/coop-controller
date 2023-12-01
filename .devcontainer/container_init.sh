#!/bin/sh

for u in $(seq 0 9)
do 
    U=/dev/ttyUSB$u
    test -e $U && echo "Setting permissions on $U" && sudo chown :dialout $U && sudo chmod g+rw $U
done


for id in 0403:6010 10c4:ea60
do 
    usb_id=$(lsusb | grep $id | awk '{ print "/dev/bus/usb/"$2"/"$4 }' | sed 's/://')
    echo "Setting permissions on $usb_id"
    sudo chmod 666 $usb_id
    sudo chown :plugdev $usb_id
done


git config --global --add safe.directory /workspace


sudo chown -R ${USERNAME} .pio ~/.platformio
python ~/get-platformio.py check core


gh extension install github/gh-copilot