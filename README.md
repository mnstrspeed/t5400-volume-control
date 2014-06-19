# T5400 volume control
The purpose of this project is to create a control interface for the Creative Inspire T5400 speaker set and the Raspberry Pi. The interface consists of a hardware component and a software component.

## Hardware
* Raspberry Pi (v1)
* LTC1661 dual 10-bit DAC, connected to the GPIO pins of the Raspberry Pi (pin layout specified in `t5400.c`)

## Software
`t5400.c` is a kernel module for Linux on the Raspberry Pi which drives the LTC1661 and allows programs in user space to control the device through a sysfs interface:

```bash
sudo make
sudo insmod t5400.ko

echo 1023 > /sys/class/volume_control/t5400/volume
echo 600 > /sys/class/volume_control/t5400/bass
...
```

Note: make sure you have installed the appropriate Linux headers, i.e.:
```bash
sudo pacman -S linux-headers
```
