#!/bin/sh
sudo modprobe -r v4l2loopback
sudo modprobe v4l2loopback devices=1 video_nr=16 card_label="aRibeiro Cam 01" max_width=1920 max_height=1080 exclusive_caps=1
