# Basic HTTP Commands; 
It's an API Jim, but not as we know it

The WebUI and camera server communicate entirely via HTTP requests and responses; this makes controlling all functions of the camera via GET requests possible. An API in effect.

## URI's
### Http Port
* `/` - Default index
* `/?view=full|simple|portal` - Go direct to specific index
* `/capture` - Return a Jpeg snapshot image
* `/status` - Returns a JSON string with all camera status <key>/<value> pairs listed
* `/control?var=<key>&val=<val>` - Set `<key>` to `<val>`
* `/dump` - Status page
* `/stop` - End all active streams

### Stream Port
* `/` - Raw stream
* `/view` - Stream viewer

## *key / val* settings and commands

Call the `/status` URI to recieve a JSON response containing all the available settings and current value.

Call `/control?var=<key>&val=<val>` with a settings key and value to set camera properties or trigger actions.

#### Settings
```
lamp            - Lamp value in percent; integer, 0 - 100 (-1 = disabled)
framesize       - See below
min_frame_time  - Minimal frame duration in ms; used to limit max FPS. Must be positive integer
quality         - 10 to 63 (ov3660: 4 to 10)
contrast        - -2 to 2 (ov3660: -3 to 3)
brightness      - -2 to 2 (ov3660: -3 to 3)
saturation      - -2 to 2 (ov3660: -4 to 4)
sharpness       - (ov3660: -3 to 3)
denoise         - (ov3660: 0 to 8)
ae_level        - (ov3660: -5 to 5)
special_effect  - 0=No Effect, 1=Negative, 2=Grayscale, 3=Red Tint, 4=Green Tint, 5=Blue Tint, 6=Sepia
awb             - 0 = disable, 1 = enable
awb_gain        - 0 = disable, 1 = enable
wb_mode         - if awb enabled: 0=Auto, 1=Sunny, 2=Cloudy, 3=Office, 4=Home
aec             - 0 = disable, 1 = enable
aec_value       - 0 to 1200 (ov3660: 0 to 1536)
aec2            - 0 = disable, 1 = enable
ae_level        - -2 to 2 (not ov3660)
agc             - 0 = disable, 1 = enable
agc_gain        - 0 to 30 (ov3660: 0 to 64)
gainceiling     - 0 to 6 (ov3660: 0 to 511)
bpc             - 0 = disable, 1 = enable
wpc             - 0 = disable, 1 = enable
raw_gma         - 0 = disable, 1 = enable
lenc            - 0 = disable, 1 = enable
hmirror         - 0 = disable, 1 = enable
vflip           - 0 = disable, 1 = enable
rotate          - Rotation Angle; integer, only -90, 0, 90 values are recognised
dcw             - 0 = disable, 1 = enable
colorbar        - Overlays a color test pattern on the stream; integer, 1 = enabled
face_detect     - Face Detection; 1 = enabled, Only settable if framesize <= 4 (CIF)
face_recognize  - Face recognition; 1 = enabled, only settable if Face detection is already enabled
```
#### Read Only
These values are returned in the `/status` JSON response, but cannot be set via the `/control` URI.
```
cam_name        - Camera Name; String
code_ver        - Code compile date and time; String
stream_url      - Raw stream URL; string
```
##### Framesize values
These may vary between different ESP framework releases
```
 0 - THUMB (96x96)
 1 - QQVGA (160x120)
 3 - HQVGA (240x176)
 5 - QVGA (320x240)
 6 - CIF (400x296)
 7 - HVGA (480x320)
 8 - VGA (640x480)
 9 - SVGA (800x600)
10 - XGA (1024x768)
11 - HD (1280x720)
12 - SXGA (1280x1024)
13 - UXGA (1600x1200)
Only for 3Mp+ camera modules:
14 - FHD (1920x1080)
17 - QXGA (2048x1536)
```
#### Commands
These are commands; they can be sent by calling the `/control` URI with them as the `<key>` *(a `<val>` must also be supplied, but can be any value and is ignored)*.
```
face_enroll     - Enroll a new face in the FaceDB (only when face recognition is avctive)
save_prefs      - Saves preferences file
clear_prefs     - Deletes the preferences file
reboot          - Reboots the camera
```
## Examples
* Flash light: on/mid/off
  * `http://<IP-ADDRESS>/control?var=lamp&val=100`
  * `http://<IP-ADDRESS>/control?var=lamp&val=50`
  * `http://<IP-ADDRESS>/control?var=lamp&val=0`
* Set resolution to VGA
  * `http://<IP-ADDRESS>/control?var=framesize&val=8`
* Show camera details and settings
  * All settings are returned via single `status` call in [JSON](https://www.json.org/) format.
  * `http://<IP-ADDRESS>/status`
  * Returns:
    ```  {"lamp":0,"autolamp":0,"min_frame_time":0,"framesize":9,"quality":10,"xclk":8,"brightness":0,"contrast":0,"saturation":0,"sharpness":0,"special_effect":0,"wb_mode":0,"awb":1,"awb_gain":1,"aec":1,"aec2":0,"ae_level":0,"aec_value":204,"agc":1,"agc_gain":0,"gainceiling":0,"bpc":0,"wpc":1,"raw_gma":1,"lenc":1,"vflip":1,"hmirror":1,"dcw":1,"colorbar":0,"cam_name":"ESP32 test camera","code_ver":"Mar 10 2022 @ 14:00:45","rotate":"0","stream_url":"http://10.0.0.181:81/"}```
* Reboot the camera
  * `http://<IP-ADDRESS>/control?var=reboot&val=0`

You can try these yourself in a browser address bar, from the commandline with `curl` and co. or use them programatically from your scripting language of choice.
