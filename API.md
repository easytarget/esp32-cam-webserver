# ESP32-CAM Web Server API
The WebUI and camera server communicate in 2 different ways:
1. Via HTTP requests and responses; this makes controlling all functions of the camera via GET requests
   possible.
2. Via a Websocket channel. There are 2 types of channels supported:
   a. Streaming channel. Used for pushing video/image data from the server to the client
   b. Control channel. Used for controlling PWM outputs on GPIO pins of the board. This may be helpful in 
      some of the use cases - PTZ servo control, for example.  

## HTTP requests and responses
### Web UI pages
* `/` Default index (camera view)
* `/view?mode=stream|still` - Go direct to specific page:
* - stream: starting video capture with full screen mode
* - still: taking a still image with full screen mode
* `/dump` - Status page (automatically refreshed every 5 sec)
* `/setup` - Configure network settings (WiFi, OTA, etc)

### Special *key / val* settings and commands

* `/control?var=<key>&val=<val>` - Set a Control Variable  specified by `<key>` to `<val>`
* `/status` - JSON response containing camera settings 
* `/system` - JSON response containing all parameters displayed on the `/dump` page

#### Supported Control Variables:
```
cmdout          - send a string to the Serial port. Allows to communicate with external devices (can be other
                  Arduino board).
lamp            - Lamp value in percent; integer, 0 - 100 (-1 = disabled). Controls the brightness of the
                  flash lamp instantly.
autolamp        - 0 = disable, 1 = enable. When set, the flash lamp will be triggered when taking the 
                  still photo. 
flashlamp       - Sets the level of the flashlamp, which will be automatically triggered at taking the still
                  image. Values are percentage integers (0-100)
framesize       - See below
frame_rate      - Frame rate in FPS. Must be positive integer. It is not reccomended to set the frame rate
                  higher than 50 FPS, otherwise the board may get unstable and stop streaming.
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
These are commands; they can be sent by calling the `/control` URI with them as 
the `<key>` parameter.
```
* save_prefs      - Saves preferences
  `val=cam` or not specified will save camera preferences
  `val=conn` will save network preferences
* remove_prefs     - Deletes camera the preferences
  `val=cam` or not specified will reset camera preferences
  `val=conn` will reset network preferences. Attention! after this the server will boot as access point after restart, and all
  connection settings will be lost. 
* reboot          - Reboots the board
```

## Examples
* Flash light: on/mid/off
  * `http://<IP-ADDRESS>/control?var=lamp&val=100`
  * `http://<IP-ADDRESS>/control?var=lamp&val=50`
  * `http://<IP-ADDRESS>/control?var=lamp&val=0`
* Set resolution to VGA
  * `http://<IP-ADDRESS>/control?var=framesize&val=8`
* Show camera details and settings
  * All settings are returned via single `status` call in [JSON](https://www.json.org/) 
    format.
  * `http://<IP-ADDRESS>/status`
  * Returns:

    ```json  
    {"cam_name":"ESP32 CAM Web Server","stream_url":"http://<ip:port>/view?mode=stream","code_ver":"Jan  7 2023 @ 19:16:55","lamp":0,"autolamp":false,"lamp":0,"flashlamp":0,"rotate":0,"xclk":8,"frame_rate":12,"framesize":8,"quality":12,"brightness":0,"contrast":0,"saturation":0,"sharpness":0,"denoise":0,"special_effect":0,"wb_mode":0,"awb":1,"awb_gain":1,"aec":1,"aec2":0,"ae_level":0,"aec_value":204,"agc":1,"agc_gain":0,"gainceiling":0,"bpc":0,"wpc":1,"raw_gma":1,"lenc":1,"vflip":0,"hmirror":0,"dcw":1,"colorbar":0,"cam_pid":38,"cam_ver":66,"debug_mode":false}
    ```
* Reboot the camera
  * `http://<IP-ADDRESS>/control?var=reboot&val=0`

You can try these yourself in a browser address bar, from the command line with `curl` 
and co. or use them programmatically from your scripting language of choice.

## ESP32CAM WebSocket API
This API is intended for fast stateful communication between the server and the browser. You can think of a websocket as a state machine, which can be accessed and programmed from the client side, using JavaScript or any other language, which supports Websocket API. 

In order to use the WebSocket API, you need to open the Websocket first. The url of the websocket is always 
`ws://<your-ip:your-port>/ws`. In Java Script, you simply need to add the following lines to your page:

```
ws = new WebSocket(websocketURL);
ws.binaryType = 'arraybuffer'; // 
```
Once the `ws` object is created successfully, you can handle its events on the page and do whatever is necessary in response. The following key events are supported:

- `onopen()`
   If you plan to create a control socket, you need to add the following command to the event handler:
   ```
   ws.send('c); // this instructs the ws that this socket client will be used for control. 
   ``` 

- `onclose()`
  This event is triggered when the client is disconnecting from the socket

- `onerror()`
   You may add some code here for handling web socket exceptions

- `onmessage()`
   This event is usually processed on the streaming websocket client. Once the stream is started, the server will be pushing the image frames and they need to be processed on the client. Here is a simple example
   of the handler for this message:

   ```
    ws.onmessage = function(event) {
      var arrayBufferView = new Uint8Array(event.data);
      var blob = new Blob([arrayBufferView], {type: "image/jpeg"});
      var imageUrl = urlCreator.createObjectURL(blob);
      video.src = imageUrl; // "video" here represents an img element on the page where frames are displayed
    }
   ```

Once the websocket is open, you may also send commands and data to the server. Commands are sent with help of the `ws.send(command)` function where the `command` is to be a binary Uint8Array.  The first byte of this 
array reflects the command code while the rest of bytes can host additional parameters of the command.

The following commands are supported:

- 's' - starts the stream. Once the command is issued, the server will start pushing the frames to the client
        according to the camera settings. The server will switch to the video mode.
- 'p' - similar to the previous command but there will be only one frame taken and pushed to the client. The
        server will switch to the photo mode.
- 'u' - similar to the previous two commands. The server will either start stream or take a still photo, 
        depending on the current mode of the server.
- 't' - terminates the stream. Only makes sense after 's' or 'u' commands.
- 'c' - tells the server that this websocket will be used for PWM control commands. 
- 'w' - writes the PWM duty value to the pin. This command has additional parameters passed in the bytes of the
        `command` array, as follows:

      byte0 - 'w' - code of the command
      byte1 - pin number. If you use the ESP32CAM-DEV board, the available pins are usually limited to 4,
              12, 13 and 33. The 4th pin is connected to the flash lamp so you can control the lamp brightness 
              by sending value to this pin via the websocket. Pin 33 is connected to the onboard LED. So, only
              12 and 13 are the ones you can use, provided that you also use the SD card for storage.
              if you use the internal LittleFS for storage, you may be able to use other pins otherwise 
              utilized by the SD card interface.
      byte2 - send 1 for servo mode and 2 for any other PWM. 
      byte3 - number of bytes in the PWM duty value, which will be written to the pin. Can be either 1 or 2
              bytes (either 8bit or 16bit value). 
      byte4 - duty value to be written to the PWM (lo-byte). For servo it can be either an angle (0-180) or a 
      byte5   value in seconds (500-2500), which will require byte5 for hi-byte of the value. 


## Attaching PWM to the GPIO pins
GPIO pins used for PWM can be defined in the `/httpd.json`, in the `pwm` parameter:

```json
{
    "my_name": "ESP32 Web Server",
    "lamp":0,
    "autolamp":true,
    "flashlamp":100,
    "pwm": [{"pin":4, "frequency":50000, "resolution":9, "default":0}],
    "mapping":[ {"uri":"/img", "path": "/www/img"},
                {"uri":"/css", "path": "/www/css"},
                {"uri":"/js", "path": "/www/js"}],
    "debug_mode": false
}
```

The `pwm` parameter is defined as a JSON array where each object of the array is a definition of one PWM.
Attributes of a pwm object are explained below:

- `pin`         - GPIO pin number
- `frequency`   - PWM frequency in Herz. 
- `resolution`  - precision of the PWM (number of bits).
- `default`     - initial value of the PWM. if this attribute is not defined,  0 will be used for default.

if the `lamp` parameter in the httpd config is greater or equal to 0, the 1st element of the pwm array
will be used for definition of flash lamp PWM. In the example above, the lamp PWM is configured for pin 4
(used by the flash lamp), 50kHz frequency, 9-bit precision.

Here is another example of the PWM configuration, used for the popular SG90 servo motor on pin 12:

```json
{
    "my_name": "ESP32 Web Server",
    "lamp":0,
    "autolamp":true,
    "flashlamp":100,
    "pwm": [{"pin":4, "frequency":50000, "resolution":9, "default":0},
            {"pin":12, "frequency":50, "resolution":10, "default": 42}],
    "mapping":[ {"uri":"/img", "path": "/www/img"},
                {"uri":"/css", "path": "/www/css"},
                {"uri":"/js", "path": "/www/js"}],
    "debug_mode": false
}
```

