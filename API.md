TODO: 
* Order the same as in the UI for most settings
* Brief Descriptions (ESP documentation?)
* ov2640 vs 3660 specifics (note them)

# Basic HTTP Commands; 
It's an API Jim, but not as we know it

The WebUI and camera server communicate entirely via HTTP requests and responses; this makes controlling all functions of the camera via GET requests possible. An API in effect.

## URI's
### Http Port
* `/` - Default index
* `/?target=full|simple|portal - Go direct to specific index
* `/capture` - Return a Jpeg snapshot image
* `/status` Returns a JSON string with all camera status <key>/<value> pairs listed
* `/control?var=<key>&val=<val>` Set <key> to <val>
* `/dump` Status page

### Stream Port
* `/` Raw stream
* `/view` Stream viewer

## *key / val* settings and commands
### Use
Call the `/status` URI to recieve a JSON response containing all the available settings and responses.

Call `/control?var=<key>&val=<val>` with a settings key and value to set camera properties or trigger actions.

#### Settings
```
framesize       - QQVGA|HQVGA|QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA|QXGA(ov3660)
quality         - 10 ro 63
contrast        - -2 to 2
brightness      - -2 to 2
saturation      - -2 to 2
gainceiling     - 0 to 6
colorbar        - Overlays a color test pattern on the stream; integer, 1 = enabled
awb             - 0 = disable , 1 = enable
agc             - 0 = disable , 1 = enable
aec             - 0 = disable , 1 = enable
hmirror         - 0 = disable , 1 = enable
vflip           - 0 = disable , 1 = enable
awb_gain        - 
agc_gain        - 0 to 30
aec_value       - 0 to 1200
aec2            - 0 = disable , 1 = enable
dcw             - 0 = disable , 1 = enable
bpc             - 0 = disable , 1 = enable
wpc             - 0 = disable , 1 = enable
raw_gma         - 0 = disable , 1 = enable
lenc            - 0 = disable , 1 = enable
special_effect  - 0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia
wb_mode         - if awb enabled: 0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home
ae_level        - -2 to 2
rotate          - Rotation Angle; integer, only -90, 0, 90 values are recognised
face_detect     - Face Detection; 1 = enabled, Only settable if framesize <= 4 (CIF)
face_recognize  - Face recognition; 1 = enabled, only settable if Face detection is already enabled
lamp            - Lamp value in percent; integer, 0 - 100 (-1 = disabled)
```
#### Read Only
These values are returned in the `/status` JSON response, but cannot be set via the `/control` URI.
```
cam_name        - Camera Name; String
code_ver        - Code compile date and time; String
stream_url      - Raw stream URL; string
```
#### Commands
These are commands; they can be sent by calling the `/control` URI with them as the `<key>`, the `<val>` must be supplied, but can be any value and is ignored.
```
face_enroll     - Enroll a new face in the FaceDB (only when face recognition is avctive)
save_prefs      - Saves preferences file
clear_prefs     - Deletes the preferences file
reboot          - Reboots the camera
```
## Examples
* Flash light: on/off
  * `http://<IP-ADDRESS>/control?var=lamp&val=100` On
  * `http://<IP-ADDRESS>/control?var=lamp&val=50` 50%
  * `http://<IP-ADDRESS>/control?var=lamp&val=0` Off

## Timelapse Example (for Linux Users)
* Install ffmpeg
* `Work this out... I've got an example somewhere`
