# ipc-v4l2loopback-source

This source uses the https://github.com/umlaeute/v4l2loopback implementation.

To load the module at boot: https://askubuntu.com/questions/1245212/how-do-i-automatically-run-modprobe-v4l2loopback-on-boot

## On Ubuntu

Install v4l2loopback tool:

```
sudo apt install v4l2loopback-utils
```

To see the module parameters

```
modinfo v4l2loopback | grep -i parm
```

To create a loopback camera you need to run:

```
sudo modprobe -r v4l2loopback
sudo modprobe v4l2loopback devices=1 video_nr=16 card_label="aRibeiro Cam 01" max_width=1920 max_height=1080 exclusive_caps=1
sudo v4l2loopback-ctl set-caps "VYUY:1920x1080" /dev/video16
sudo v4l2loopback-ctl set-caps video/x-raw,format=VYUY,width=1920,height=1080 /dev/video16
```

Check the device creation:
```
v4l2-ctl --list-devices
```

After that the tool can be executed to feed the camera buffer.
