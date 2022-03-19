sudo apt install v4l2loopback-utils
sudo cp etc/modules-load.d/v4l2loopback.conf /etc/modules-load.d/
sudo cp etc/modprobe.d/v4l2loopback.conf /etc/modprobe.d/
sudo update-initramfs -u
sudo modprobe -r v4l2loopback
sudo modprobe v4l2loopback devices=1 video_nr=16 card_label="aRibeiro Cam 01" max_width=1920 max_height=1080
