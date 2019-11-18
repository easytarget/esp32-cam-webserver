# Espressives ESP32-CAM example revisited.
## Taken from the ESP examples, and modified for reality
This sketch is a extension/expansion/rework of the 'official' ESP32 Camera example sketch from Expressif:
https://github.com/espressif/arduino-esp32/tree/master/libraries/ESP32/examples/Camera/CameraWebServer
- The example they have is nice, but a bit incomprehensible and hard to modify as supplied. It is very focused on showing off the face recognition capabilities, and forgets the 'webcam' part. 

I believe this expanded example is more useful for those users who wish to set up a simple ESP32 based webcam using the cheap(ish) modules freely available online. Especially the AI-THINKER board:
https://wiki.ai-thinker.com/esp32-cam

### Also:
https://github.com/raphaelbs/esp32-cam-ai-thinker

## Specifics:
The basic example is extended to allow control of a high power LED FlashLamps like the ones used on mobile phones, which are present on some modules. It can also blink a status LED to show when it connects to WiFi.

The WiFi details can be stored in an (optional) header file to allow easier further development, and a camera name for the UI title can be configured. The lamp and status LED's are optional, and the lamp uses a exponential scale for brightness so that the control has some finess.

The compressed and binary encoded HTML used in the example has been unpacked to raw text, this makes it much easier to access and modify the Javascript and UI elements. Given the relatively small size of the index page there is very little benefit from compressing it, .

I have left all the Face Recognition code untouched, it works, and with good lighting and camera position it can work quite well. But you can only use it in low-resolution modes, and it is not something I wil be using.

The web UI has had minor changes to add the lamp control (only when enabled), I also made the 'Start Stream' and 'Snapshot' controls more prominent, and added feedback of the camera name + firmware.

## Use:
* arduino IDE, espressive toolchain, board
* wiring
* programming

#### Config

## Plans
* SD/TF card; store snapshots etc.
* Wifi, captive portal for setup and fallback, better disconnect/reconnect behaviour.
* Remove face rcognition to savee a Mb+ of code space and then implement over the air updates.
* Combine current split html pages (one per camera type) into one which adapts as needed.

## Notes: 
* I only have AI-THINKER modules with OV2640 camera installed; so I have only been able to test with this combination. I have attempted to preserve all the code for other boards and the OV3660 module, and I have merged all changes for the WebUI etc, but I cannot guarantee operation for these.
* devboard
* cases on Thingieverse