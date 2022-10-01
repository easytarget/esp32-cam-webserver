# ESP32-CAM Example Revisited<sup>2</sup> &nbsp;&nbsp;&nbsp;  &nbsp;&nbsp; <span title="ESP EYE">![ESP-EYE logo](data/www/logo.svg)</span>

## Taken from the ESP examples, and expanded 
This sketch is a extension/expansion/rework of the 'official' ESP32 Camera example 
sketch from Espressif:

* Original [CameraWebServer](https://github.com/espressif/arduino-esp32/tree/master/libraries/ESP32/examples/Camera/CameraWebServer)

### Key features: ###
* Extended options for default network and camera settings
* Save and restore settings in JSON configuration files
* Dedicated standalone stream viewer
* Over The Air firmware updates
* Optimizing the way how the video stream is processed, thus allowing higher frame rates on high resolution.
* Using just one IP port instead of two. 
* Porting the web server to [ESP Async Web Server](https://github.com/me-no-dev/ESPAsyncWebServer). 
* Storing web pages as separate HTML/CSS/JS files on the SD drive, which greatly simplifies development of the interface. Basically, you can swap the face of this project just by replacing files on SD card. 
* Reduced size of the sketch and improving memory utilization
* Porting the code from basic C to C++ object hierarchy, eliminating extensive use of global variables 
* Lots of minor fixes and tweaks, documentation etc.

### Key principles ###
There are many other variants of a webcam server for these modules online, 
but most are created for a specific scenario and not good for general, casual, 
webcam use.

Hopefully this expanded example is more useful for those users who wish to set up 
a simple ESP32 based webcam using the cheap(ish) modules freely available online.

### Summary of reductions ###
When re-desiginig and refactoring the original ESP32 Camera web server example from
Espressve, the following key principles were followed:

1. Any idea can be killed by unnecessary features 
2. See [this tutorial video](https://www.youtube.com/watch?v=iMULJIXPxK4).

Given the above, face recognition feature was removed. The main purpose of this 
sketch is to make the camera web server easily configurable and reusable. 

The original example, is a bit incomprehensible and hard to modify as supplied. 
It is very focused on showing off the face recognition capabilities, and forgets 
the 'webcam' part.
  
### Supported development boards ###
The sketch has been tested on the [AI Thinker ESP32-CAM](https://github.com/raphaelbs/esp32-cam-ai-thinker/blob/master/assets/ESP32-CAM_Product_Specification.pdf) 
module. Other ESP32 boards equipped with camera may be compatible but not guaranteed.

### Known Issues

The ESP32 itself is susceptible to the usual list of WiFi problems, not helped by having 
small antennas, older designs, congested airwaves and demanding users. The majority of 
disconnects, stutters and other communication problems are simply due to 'WiFi issues'. 

The AI-THINKER camera module & esp32 combination is quite susceptible to power supply 
problems affecting both WiFi conctivity and Video quality; short cabling and decent 
power supplies are your friend here; also well cooled cases and, if you have the time, 
decoupling capacitors on the power lines.

A basic limitation of the sketch is that it can can only support one stream at a time. 
If you try to connect to a cam that is already streaming (or attempting to stream, 
the first steam will freeze. 

Currently, camera modules other than ov2640 are not supported.

## Setup:

* For programming you will need a suitable development environment. Possible options
  include Visual Studio Code, Arduino Studio or Espressif development environment .

### Wiring for AI-THINKER Boards (and similar clone-alikes)

Is pretty simple, You just need jumper wires, no soldering really required, see the diagram below.
![Hoockup](Docs/hookup.png)
* Connect the **RX** line from the serial adapter to the **TX** pin on ESP32
* The adapters **TX** line goes to the ESP32 **RX** pin
* The **GPIO0** pin of the ESP32 must be held LOW (to ground) when the unit is 
  powered up to allow it to enter it's programming mode. This can be done with simple 
  jumper cable connected at poweron, fitting a switch for this is useful if you 
  will be reprogramming a lot.
* You will to supply 5v to the ESP32 in order to power it during programming; the FTDI 
  board alone fails to supply this sometimes. The ESP32 CAM board is very sensitive 
  to the quality of power source. Decoupling capacitors are very much recommended.

### Download the Sketch, Unpack and Rename
Download the latest release of the sketch this repository. Once you have done that you 
can open the sketch in the IDE by going to the `esp32-cam-webserver` sketch folder and 
selecting `esp32-cam-webserver.ino`.

You also need to copy the content of the **data** folder from this repository to a micro 
SD flash memory card (must be formatted as FAT32) and insert it into the micro SD slot of 
the board. 

Without the SD card, the sketch will not start. Please ensure the size of the card does 
not exceed 4GB, which is a maximum supported capacity for ESP32-CAM board. 
Higher capacity SD card may not work.

### Config

You will need to configure the web server with your WiFi settings. In order to do so,
you will need to create a config file in the root folder of your SD card named `conn.json`
and format it as follows:

```json
{   
    "mdns_name":"YOUR_MDNS_NAME",
    "stations":[
        {"ssid": "YOUR_SSID", "pass":"YOUR_WIFI_PASSWORD", "dhcp": true}
    ],
    "http_port":80,
    "ota_enabled":true,
    "ota_password":"YOUR_OTA_PASSWORD",
    "ap_ssid":"esp32cam",
    "ap_pass":"123456789",
    "ap_ip": {"ip":"192.168.4.1", "netmask":"255.255.255.0"},
    "ap_dhcp":true,
    "ntp_server":"pool.ntp.org",
    "gmt_offset":14400,
    "dst_offset":0,
    "debug_mode": false
}
```
Replace the WiFi and OTA parameters with your settings and save. PLease note that the sketch
will not boot properly if WiFi connection is established.

Web server name can configured by creating another config file, `httpd.json`, in the root
folder of the SD card:

```json
{
    "my_name": "MY_NAME",
    "debug_mode": false
}
```

Similarly, default camera configuration parameters can be set by creating the file `cam.json`:

```json
{   
    "lamp":-1,
    "autolamp":0,
    "framesize":8,
    "quality":12,
    "xclk":8,
    "frame_rate":25,
    "brightness":0,
    "contrast":0,
    "saturation":0,
    "special_effect":0,
    "wb_mode":0,"awb":1,
    "awb_gain":1,
    "aec":1,
    "aec2":0,
    "ae_level":0,
    "aec_value":204,
    "agc":1,
    "agc_gain":0,
    "gainceiling":0,
    "bpc":0,
    "wpc":1,
    "raw_gma":1,
    "lenc":1,
    "vflip":0,
    "hmirror":0,
    "dcw":1,
    "colorbar":0,
    "rotate":"0", 
    "debug_mode": false
}
```

### Programming

Assuming you are using the latest Espressif Arduino core the `ESP32 Dev Module` board 
will appear in the ESP32 Arduino section of the boards list. Select this (do not use 
the `AI-THINKER` entry listed in the boiards menu, it is not OTA compatible, and will 
cause the module to crash and reboot rather than updating if you use it.
![IDE board config](Docs/ota-board-selection.png)

Make sure you select the `Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)` partition 
cheme and turn `PSRAM` on.

The first time you program (or if OTA is failing) you need to compile and upload the 
code from the IDE, and when the `Connecting...` appears in the console reboot the ESP32 
module while keeping **GPIO0** grounded. You can release GPO0 once the sketch is 
uploading, most boards have a 'boot' button to trigger a reboot.

Once the upload completes (be patient, it can be a bit slow) open the serial monitor 
in the IDE and reboot the board again without GPIO0 grounded. In the serial monitor 
you should see the board start, connect to the wifi and then report the IP address 
it has been assigned.

Once you have the initial upload done and the board is connected to the wifi network 
you should see it appearing in the `network ports` list of the IDE, and you can upload 
wirelessly.

If you have a status LED configured it will give a double flash when it begins 
attempting to conenct to WiFi, and five short flashes once it has succeeded. It will 
also flash briefly when you access the camera to change settings.

Go to the URL given in the serial output, the web UI should appear with the settings 
panel open. Click away!

### API
The communications between the web browser and the camera module can also be used to 
send commands directly to the camera (eg to automate it, etc) and form, in effect, 
an API for the camera.
* [ESP32 Camera Web Server JSON API](API.md).

## Contributing

Contributions are welcome; please see the [Contribution guidelines](CONTRIBUTING.md).

## Future plans

1. Support of LittleFS. 
3. Support of other boards and cameras.
4. Explore how to improve the video quality and further reduce requirements to resources.

