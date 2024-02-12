#!/bin/sh
sudo rm /etc/modules-load.d/OpenMultimedia_v4l2loopback.conf
sudo rm /etc/modprobe.d/OpenMultimedia_v4l2loopback.conf
sudo update-initramfs -u
sudo modprobe -r v4l2loopback
