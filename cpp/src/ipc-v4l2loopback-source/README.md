# ipc-v4l2loopback-source

This source uses the https://github.com/umlaeute/v4l2loopback implementation.

To load the module at boot: https://askubuntu.com/questions/1245212/how-do-i-automatically-run-modprobe-v4l2loopback-on-boot

## On Ubuntu

Install v4l2loopback tool:

```bash
sudo apt install v4l2loopback-utils
```

To see the module parameters

```bash
modinfo v4l2loopback | grep -i parm
```

To create a loopback camera you need to run:

```bash
# normal to OBS
sudo modprobe -r v4l2loopback && 
sudo modprobe v4l2loopback devices=1 video_nr=16 card_label="aRibeiro Cam 01" max_width=1920 max_height=1080

# normal to chrome
sudo modprobe -r v4l2loopback && 
sudo modprobe v4l2loopback devices=1 video_nr=16 card_label="aRibeiro Cam 01" max_width=1920 max_height=1080 exclusive_caps=1
```

Check the device creation:
```
v4l2-ctl --list-devices
```

After that the tool can be executed to feed the camera buffer.
