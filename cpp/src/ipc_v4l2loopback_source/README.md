# ipc-v4l2loopback-source

This source uses the https://github.com/umlaeute/v4l2loopback implementation.

To create a loopback camera you need to run:

```
sudo v4l2loopback-ctl delete /dev/video16
sudo v4l2loopback-ctl add -n "aRibeiro Cam 01" /dev/video16
sudo v4l2loopback-ctl set-caps /dev/video16 "YUYV:1920x1080"
```

After that the tool can be executed to feed the camera buffer.
